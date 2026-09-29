#ifndef SERVERCONNECTOR_H
#define SERVERCONNECTOR_H

#include <mutex>
#include <optional>
#include <string>

#include "base.pb.h"
#include "hazkey/frontend/server_connection.h"

// Starts hazkey-server with QProcess. Logs are not shown.
class SettingsConnectionHooks : public hazkey::frontend::ConnectionHooks {
   public:
    void startServer(bool forceRestart) override;
    void log(hazkey::frontend::LogLevel, const std::string&) override {}
};

class ServerConnector {
   public:
    ServerConnector();
    ~ServerConnector();
    std::optional<hazkey::config::CurrentConfig> getConfig();
    void setCurrentConfig(hazkey::config::CurrentConfig);
    bool clearAllHistory(const std::string& profileId);
    bool reloadZenzaiModel();

    // Begin a session with persistent connection
    bool beginSession();
    // End the session and close connection
    void endSession();
    // Session-aware versions of methods
    std::optional<hazkey::config::CurrentConfig> getConfigInSession();
    bool reloadZenzaiModelInSession();

   private:
    // Connect for this transaction only.
    std::optional<hazkey::ResponseEnvelope> transact(
        const hazkey::RequestEnvelope& send_data);
    std::optional<hazkey::ResponseEnvelope> transactInSession(
        const hazkey::RequestEnvelope& send_data);

    SettingsConnectionHooks hooks_;
    std::mutex mutex_;
    std::optional<hazkey::frontend::ServerConnection> session_;
};

#endif  // SERVERCONNECTOR_H
