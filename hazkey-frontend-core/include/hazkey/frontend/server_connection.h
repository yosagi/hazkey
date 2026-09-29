#ifndef HAZKEY_FRONTEND_SERVER_CONNECTION_H_
#define HAZKEY_FRONTEND_SERVER_CONNECTION_H_

#include <mutex>
#include <optional>
#include <string>

#include "base.pb.h"

namespace hazkey::frontend {

enum class LogLevel { Debug, Info, Error };

// Client specific parts of the connection: how to start hazkey-server and
// where to write logs.
class ConnectionHooks {
   public:
    virtual ~ConnectionHooks() = default;

    // Start hazkey-server in the background. forceRestart passes -r, which
    // replaces a running server.
    virtual void startServer(bool forceRestart) = 0;
    virtual void log(LogLevel level, const std::string& message) = 0;
};

// How connect() retries. Attempts are counted from 0.
struct ConnectPolicy {
    int maxRetries = 8;
    int retryIntervalMs = 250;
    // start the server after this failed attempt
    int startAttempt = 0;
    // start the server with -r after this failed attempt (-1: never)
    int forceRestartAttempt = -1;
};

// Socket path of hazkey-server (same rule as the server).
std::string serverSocketPath();

// Start hazkey-server detached from the caller (double fork + exec), so it
// is neither left as a zombie nor tied to the caller's lifetime.
void spawnServerDetached(bool forceRestart);

// Connection to hazkey-server. Messages are a 4 byte length (network order)
// followed by a serialized protobuf envelope.
class ServerConnection {
   public:
    ServerConnection(ConnectionHooks& hooks, ConnectPolicy policy = {});
    ~ServerConnection();

    ServerConnection(const ServerConnection&) = delete;
    ServerConnection& operator=(const ServerConnection&) = delete;

    // Connect with retries, starting the server as the policy says.
    bool connect();
    bool isConnected() const;
    void disconnect();

    // Send a request and wait for the response. When not connected, connects
    // first if tryConnect is true. A failure while writing reconnects for the
    // next call (if tryConnect), a failure while reading just disconnects.
    std::optional<hazkey::ResponseEnvelope> transact(
        const hazkey::RequestEnvelope& request, bool tryConnect = true);

   private:
    bool connectLocked();
    void closeSocket();

    ConnectionHooks& hooks_;
    ConnectPolicy policy_;
    std::mutex mutex_;
    int sock_ = -1;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_SERVER_CONNECTION_H_
