#include "serverconnector.h"

#include <QProcess>
#include <QStringList>

namespace {
// Start the server after the 1st failed attempt, with -r after the 4th.
constexpr hazkey::frontend::ConnectPolicy kConnectPolicy{
    /*maxRetries=*/8, /*retryIntervalMs=*/250, /*startAttempt=*/0,
    /*forceRestartAttempt=*/3};
}  // namespace

void SettingsConnectionHooks::startServer(bool forceRestart) {
    QStringList args;
    if (forceRestart) {
        args << "-r";
    }
    QProcess::startDetached("hazkey-server", args, "/");
}

ServerConnector::ServerConnector() {}

ServerConnector::~ServerConnector() { endSession(); }

std::optional<hazkey::ResponseEnvelope> ServerConnector::transact(
    const hazkey::RequestEnvelope& send_data) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create new connection for each transaction
    hazkey::frontend::ServerConnection connection(hooks_, kConnectPolicy);
    if (!connection.connect()) {
        return std::nullopt;
    }
    return connection.transact(send_data, false);
}

std::optional<hazkey::ResponseEnvelope> ServerConnector::transactInSession(
    const hazkey::RequestEnvelope& send_data) {
    if (!session_ || !session_->isConnected()) {
        return std::nullopt;
    }
    return session_->transact(send_data, false);
}

bool ServerConnector::beginSession() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Close existing session if any
    session_.reset();
    session_.emplace(hooks_, kConnectPolicy);
    return session_->connect();
}

void ServerConnector::endSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    session_.reset();
}

std::optional<hazkey::config::CurrentConfig>
ServerConnector::getConfigInSession() {
    std::lock_guard<std::mutex> lock(mutex_);

    hazkey::RequestEnvelope request;
    auto _ = request.mutable_get_config();
    auto response = transactInSession(request);
    if (response == std::nullopt) {
        return std::nullopt;
    }
    auto responseVal = response.value();
    if (responseVal.status() != hazkey::SUCCESS) {
        return std::nullopt;
    }
    if (!responseVal.has_current_config()) {
        return std::nullopt;
    }
    return responseVal.current_config();
}

bool ServerConnector::reloadZenzaiModelInSession() {
    std::lock_guard<std::mutex> lock(mutex_);

    hazkey::RequestEnvelope request;
    auto _ = request.mutable_reload_zenzai_model();
    auto response = transactInSession(request);
    if (response == std::nullopt) {
        return false;
    }
    auto responseVal = response.value();
    return responseVal.status() == hazkey::SUCCESS;
}

std::optional<hazkey::config::CurrentConfig> ServerConnector::getConfig() {
    hazkey::RequestEnvelope request;
    auto _ = request.mutable_get_config();
    auto response = transact(request);
    if (response == std::nullopt) {
        return std::nullopt;
    }
    auto responseVal = response.value();
    if (responseVal.status() != hazkey::SUCCESS) {
        return std::nullopt;
    }
    if (!responseVal.has_current_config()) {
        return std::nullopt;
    }
    return responseVal.current_config();
}

void ServerConnector::setCurrentConfig(
    hazkey::config::CurrentConfig currentConfig) {
    hazkey::RequestEnvelope request;
    auto props = request.mutable_set_config();
    *props->mutable_profiles() = currentConfig.profiles();
    auto response = transact(request);
    if (response == std::nullopt) {
        return;
    }
    auto responseVal = response.value();
    if (responseVal.status() != hazkey::SUCCESS) {
        return;
    }
}

bool ServerConnector::clearAllHistory(const std::string& profileId) {
    hazkey::RequestEnvelope request;
    auto clearRequest = request.mutable_clear_all_history();
    clearRequest->set_profile_id(profileId);
    auto response = transact(request);
    if (response == std::nullopt) {
        return false;
    }
    auto responseVal = response.value();
    return responseVal.status() == hazkey::SUCCESS;
}

bool ServerConnector::reloadZenzaiModel() {
    hazkey::RequestEnvelope request;
    auto _ = request.mutable_reload_zenzai_model();
    auto response = transact(request);
    if (response == std::nullopt) {
        return false;
    }
    auto responseVal = response.value();
    return responseVal.status() == hazkey::SUCCESS;
}
