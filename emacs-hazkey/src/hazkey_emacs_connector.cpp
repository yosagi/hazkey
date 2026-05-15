#include "hazkey_emacs_connector.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

static std::mutex transact_mutex;

static bool writeAll(int fd, const void* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, (const char*)data + sent, len - sent);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(fd, &wfds);
                timeval tv = {2, 0};
                int r = select(fd + 1, NULL, &wfds, NULL, &tv);
                if (r <= 0) return false;
                continue;
            }
            return false;
        }
        sent += n;
    }
    return true;
}

static bool readAll(int fd, void* data, size_t len) {
    size_t recved = 0;
    while (recved < len) {
        ssize_t n = read(fd, (char*)data + recved, len - recved);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
                timeval tv = {10, 0};
                int r = select(fd + 1, &rfds, NULL, NULL, &tv);
                if (r <= 0) return false;
                continue;
            }
            return false;
        }
        if (n == 0) return false;
        recved += n;
    }
    return true;
}

std::string HazkeyEmacsConnector::runtimeDir() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    uid_t uid = getuid();
    std::string base;
    if (xdg && xdg[0] != '\0') {
        base = std::string(xdg);
    } else {
        base = "/tmp/hazkey-runtime-" + std::to_string(uid);
    }
    return base + "/hazkey-emacs";
}

void HazkeyEmacsConnector::ensureDedicatedServer() {
    std::string dir = runtimeDir();
    mkdir(dir.c_str(), 0700);

    std::string socketPath = getSocketPath();
    if (access(socketPath.c_str(), F_OK) == 0) {
        // Socket file exists, server might be running
        return;
    }

    fprintf(stderr, "hazkey_emacs_helper: starting dedicated hazkey-server...\n");

    pid_t pid = fork();
    if (pid == 0) {
        // Child: start hazkey-server with custom XDG_RUNTIME_DIR
        setenv("XDG_RUNTIME_DIR", dir.c_str(), 1);

        // Redirect stdout/stderr to /dev/null to avoid polluting emacs pipe
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        execlp("hazkey-server", "hazkey-server", "-r", nullptr);
        _exit(127);
    } else if (pid > 0) {
        serverPid_ = pid;
        // Wait for server to start (check socket appears)
        for (int i = 0; i < 40; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (access(socketPath.c_str(), F_OK) == 0) {
                fprintf(stderr, "hazkey_emacs_helper: dedicated server started (pid %d)\n", pid);
                return;
            }
        }
        fprintf(stderr, "hazkey_emacs_helper: server start timeout\n");
    } else {
        fprintf(stderr, "hazkey_emacs_helper: fork failed\n");
    }
}

HazkeyEmacsConnector::HazkeyEmacsConnector() {
    ensureDedicatedServer();
    connectServer();
}

HazkeyEmacsConnector::~HazkeyEmacsConnector() {
    if (sock_ >= 0) close(sock_);
    if (serverPid_ > 0) {
        kill(serverPid_, SIGTERM);
        int status;
        waitpid(serverPid_, &status, WNOHANG);
    }
}

std::string HazkeyEmacsConnector::getSocketPath() {
    uid_t uid = getuid();
    return runtimeDir() + "/hazkey-server." + std::to_string(uid) + ".sock";
}

void HazkeyEmacsConnector::connectServer() {
    std::string socket_path = getSocketPath();
    constexpr int MAX_RETRIES = 8;
    constexpr int RETRY_INTERVAL_MS = 250;

    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_ < 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(RETRY_INTERVAL_MS));
            continue;
        }
        fcntl(sock_, F_SETFL, fcntl(sock_, F_GETFL, 0) | O_NONBLOCK);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

        int ret = connect(sock_, (sockaddr*)&addr, sizeof(addr));
        if (ret == 0) return;
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
                if (so_error == 0) return;
            }
        }
        close(sock_);
        sock_ = -1;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(RETRY_INTERVAL_MS));
    }
    fprintf(stderr, "hazkey_emacs_helper: failed to connect to hazkey-server\n");
}

std::optional<hazkey::ResponseEnvelope> HazkeyEmacsConnector::transact(
    const hazkey::RequestEnvelope& send_data, bool tryConnect) {
    std::lock_guard<std::mutex> lock(transact_mutex);

    if (sock_ == -1) {
        if (!tryConnect) return std::nullopt;
        connectServer();
        if (sock_ == -1) return std::nullopt;
    }

    std::string msg;
    if (!send_data.SerializeToString(&msg)) return std::nullopt;

    uint32_t writeLen = htonl(msg.size());
    if (!writeAll(sock_, &writeLen, 4)) {
        close(sock_);
        sock_ = -1;
        if (tryConnect) connectServer();
        return std::nullopt;
    }
    if (!writeAll(sock_, msg.c_str(), msg.size())) {
        close(sock_);
        sock_ = -1;
        if (tryConnect) connectServer();
        return std::nullopt;
    }

    uint32_t readLenBuf;
    if (!readAll(sock_, &readLenBuf, 4)) {
        close(sock_);
        sock_ = -1;
        return std::nullopt;
    }
    uint32_t readLen = ntohl(readLenBuf);
    if (readLen > 2 * 1024 * 1024) {
        close(sock_);
        sock_ = -1;
        return std::nullopt;
    }

    std::vector<char> buf(readLen);
    if (!readAll(sock_, buf.data(), readLen)) {
        close(sock_);
        sock_ = -1;
        return std::nullopt;
    }

    hazkey::ResponseEnvelope resp;
    if (!resp.ParseFromArray(buf.data(), readLen)) return std::nullopt;
    return resp;
}

std::string HazkeyEmacsConnector::getComposingText(
    hazkey::commands::GetComposingString::CharType type,
    const std::string& currentPreedit) {
    hazkey::RequestEnvelope request;
    auto props = request.mutable_get_composing_string();
    props->set_char_type(type);
    props->set_current_preedit(currentPreedit);
    auto response = transact(request);
    if (!response || response->status() != hazkey::SUCCESS) return "";
    return response->text();
}

TextWithCursor HazkeyEmacsConnector::getComposingHiraganaWithCursor() {
    hazkey::RequestEnvelope request;
    request.mutable_get_hiragana_with_cursor();
    auto response = transact(request);
    if (!response || response->status() != hazkey::SUCCESS ||
        !response->has_text_with_cursor())
        return {};
    return {response->text_with_cursor().beforecursosr(),
            response->text_with_cursor().oncursor(),
            response->text_with_cursor().aftercursor()};
}

void HazkeyEmacsConnector::inputChar(const std::string& text) {
    hazkey::RequestEnvelope request;
    request.mutable_input_char()->set_text(text);
    transact(request);
}

void HazkeyEmacsConnector::shiftKeyEvent(bool isRelease) {
    hazkey::RequestEnvelope request;
    auto props = request.mutable_modifier_event();
    props->set_event_type(isRelease
                              ? hazkey::commands::ModifierEvent_EventType_RELEASE
                              : hazkey::commands::ModifierEvent_EventType_PRESS);
    props->set_mod_type(hazkey::commands::ModifierEvent_ModifierType_SHIFT);
    transact(request);
}

bool HazkeyEmacsConnector::currentInputModeIsDirect() {
    hazkey::RequestEnvelope request;
    request.mutable_get_current_input_mode();
    auto response = transact(request);
    if (!response || response->status() != hazkey::SUCCESS) return false;
    return response->current_input_mode_info().input_mode() ==
           hazkey::commands::CurrentInputModeInfo_InputMode_DIRECT;
}

void HazkeyEmacsConnector::deleteLeft() {
    hazkey::RequestEnvelope request;
    request.mutable_delete_left();
    transact(request);
}

void HazkeyEmacsConnector::deleteRight() {
    hazkey::RequestEnvelope request;
    request.mutable_delete_right();
    transact(request);
}

void HazkeyEmacsConnector::moveCursor(int offset) {
    hazkey::RequestEnvelope request;
    request.mutable_move_cursor()->set_offset(offset);
    transact(request);
}

void HazkeyEmacsConnector::setContext(const std::string& context, int anchor) {
    hazkey::RequestEnvelope request;
    auto props = request.mutable_set_context();
    props->set_context(context);
    props->set_anchor(anchor);
    transact(request);
}

void HazkeyEmacsConnector::newComposingText() {
    hazkey::RequestEnvelope request;
    request.mutable_new_composing_text();
    transact(request);
}

void HazkeyEmacsConnector::completePrefix(int index) {
    hazkey::RequestEnvelope request;
    request.mutable_prefix_complete()->set_index(index);
    transact(request);
}

void HazkeyEmacsConnector::saveLearningData(bool tryConnect) {
    hazkey::RequestEnvelope request;
    request.mutable_save_learning_data();
    transact(request, tryConnect);
}

hazkey::commands::CandidatesResult HazkeyEmacsConnector::getCandidates(
    bool isSuggest) {
    hazkey::RequestEnvelope request;
    request.mutable_get_candidates()->set_is_suggest(isSuggest);
    auto response = transact(request);
    if (!response || response->status() != hazkey::SUCCESS)
        return hazkey::commands::CandidatesResult();
    return response->candidates();
}
