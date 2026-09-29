#include "hazkey/frontend/server_connection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

namespace hazkey::frontend {

namespace {

constexpr uint32_t MAX_RESPONSE_SIZE = 2 * 1024 * 1024;  // 2MB

bool writeAll(int fd, const void* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, (const char*)data + sent, len - sent);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(fd, &wfds);
                timeval tv = {2, 0};  // 2sec timeout
                int r = select(fd + 1, NULL, &wfds, NULL, &tv);
                if (r <= 0) {
                    return false;
                }
                continue;
            }
            return false;
        }
        sent += n;
    }
    return true;
}

bool readAll(int fd, void* data, size_t len) {
    size_t recved = 0;
    while (recved < len) {
        ssize_t n = read(fd, (char*)data + recved, len - recved);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
                timeval tv = {10, 0};  // 10sec timeout
                int r = select(fd + 1, &rfds, NULL, NULL, &tv);
                if (r <= 0) {
                    return false;
                }
                continue;
            }
            return false;
        }
        if (n == 0) return false;  // closed
        recved += n;
    }
    return true;
}

}  // namespace

std::string serverSocketPath() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    uid_t uid = getuid();
    std::string dir;
    if (xdg && xdg[0] != '\0') {
        dir = xdg;
    } else {
        dir = "/tmp/hazkey-runtime-" + std::to_string(uid);
    }
    return dir + "/hazkey-server." + std::to_string(uid) + ".sock";
}

void spawnServerDetached(bool forceRestart) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        if (fork() != 0) _exit(0);

        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        if (forceRestart) {
            execlp("hazkey-server", "hazkey-server", "-r", nullptr);
        } else {
            execlp("hazkey-server", "hazkey-server", nullptr);
        }
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
}

ServerConnection::ServerConnection(ConnectionHooks& hooks,
                                   ConnectPolicy policy)
    : hooks_(hooks), policy_(policy) {}

ServerConnection::~ServerConnection() { closeSocket(); }

bool ServerConnection::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectLocked();
}

bool ServerConnection::isConnected() const { return sock_ != -1; }

void ServerConnection::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    closeSocket();
}

void ServerConnection::closeSocket() {
    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }
}

bool ServerConnection::connectLocked() {
    closeSocket();
    std::string socket_path = serverSocketPath();

    for (int attempt = 0; attempt < policy_.maxRetries; ++attempt) {
        sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_ < 0) {
            hooks_.log(LogLevel::Error, "Failed to create socket");
            sock_ = -1;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(policy_.retryIntervalMs));
            continue;
        }
        int fcntlRes =
            fcntl(sock_, F_SETFL, fcntl(sock_, F_GETFL, 0) | O_NONBLOCK);
        if (fcntlRes != 0) {
            hooks_.log(LogLevel::Error, "fcntl() failed");
            closeSocket();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(policy_.retryIntervalMs));
            continue;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

        int ret = ::connect(sock_, (sockaddr*)&addr, sizeof(addr));
        if (ret == 0) {
            return true;
        }
        if (errno == EINPROGRESS) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(sock_, &wfds);
            timeval tv = {2, 0};
            int sel = select(sock_ + 1, NULL, &wfds, NULL, &tv);
            if (sel > 0 && FD_ISSET(sock_, &wfds)) {
                int so_error = 0;
                socklen_t len = sizeof(so_error);
                getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    return true;
                }
            }
        }
        hooks_.log(LogLevel::Info, "Failed to connect hazkey-server, retry " +
                                       std::to_string(attempt + 1));
        closeSocket();
        if (attempt == policy_.startAttempt) {
            hooks_.startServer(false);
        } else if (attempt == policy_.forceRestartAttempt) {
            hooks_.startServer(true);
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(policy_.retryIntervalMs));
    }
    hooks_.log(LogLevel::Info, "Failed to connect hazkey-server after " +
                                   std::to_string(policy_.maxRetries) +
                                   " attempts");
    return false;
}

std::optional<hazkey::ResponseEnvelope> ServerConnection::transact(
    const hazkey::RequestEnvelope& request, bool tryConnect) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (sock_ == -1) {
        if (!tryConnect) {
            hooks_.log(LogLevel::Info,
                       "Socket not connected. Aborting transact.");
            return std::nullopt;
        }
        hooks_.log(LogLevel::Info,
                   "Socket not connected, attempting to connect...");
        if (!connectLocked()) {
            hooks_.log(LogLevel::Error,
                       "Failed to establish connection to hazkey-server");
            return std::nullopt;
        }
    }

    std::string msg;
    if (!request.SerializeToString(&msg)) {
        hooks_.log(LogLevel::Error, "Failed to serialize protobuf message.");
        return std::nullopt;
    }

    hooks_.log(LogLevel::Debug,
               "Sending message of size: " + std::to_string(msg.size()));

    uint32_t writeLen = htonl(msg.size());
    if (!writeAll(sock_, &writeLen, 4) ||
        !writeAll(sock_, msg.c_str(), msg.size())) {
        closeSocket();
        if (tryConnect) {
            hooks_.log(LogLevel::Info,
                       "Failed to communicate with server while writing. "
                       "reconnecting to hazkey-server...");
            connectLocked();
        }
        return std::nullopt;
    }

    uint32_t readLenBuf;
    if (!readAll(sock_, &readLenBuf, 4)) {
        hooks_.log(LogLevel::Error, "Failed to read buffer length.");
        closeSocket();
        return std::nullopt;
    }

    uint32_t readLen = ntohl(readLenBuf);
    if (readLen > MAX_RESPONSE_SIZE) {
        hooks_.log(LogLevel::Error,
                   "Response size too large: " + std::to_string(readLen));
        closeSocket();
        return std::nullopt;
    }

    std::vector<char> buf(readLen);
    if (!readAll(sock_, buf.data(), readLen)) {
        hooks_.log(LogLevel::Error, "Failed to read response body.");
        closeSocket();
        return std::nullopt;
    }

    hazkey::ResponseEnvelope resp;
    if (!resp.ParseFromArray(buf.data(), readLen)) {
        hooks_.log(LogLevel::Error, "Failed to parse received data");
        return std::nullopt;
    }
    return resp;
}

}  // namespace hazkey::frontend
