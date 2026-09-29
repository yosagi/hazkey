#include "hazkey_engine.h"

#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/misc.h>

#include <string>
#include <vector>

#include "hazkey_state.h"
#include "hazkey_constants.h"

namespace fcitx {

namespace {
// Start the server after the 1st failed attempt, with -r after the 4th.
constexpr hazkey::frontend::ConnectPolicy kConnectPolicy{
    /*maxRetries=*/8, /*retryIntervalMs=*/250, /*startAttempt=*/0,
    /*forceRestartAttempt=*/3};
}  // namespace

void FcitxConnectionHooks::startServer(bool forceRestart) {
    std::vector<std::string> args;
    args.push_back("hazkey-server");
    if (forceRestart) {
        args.push_back("-r");
    }
    startProcess(args, "/");
}

void FcitxConnectionHooks::log(hazkey::frontend::LogLevel level,
                               const std::string &message) {
    switch (level) {
        case hazkey::frontend::LogLevel::Debug:
            FCITX_DEBUG() << message;
            break;
        case hazkey::frontend::LogLevel::Info:
            FCITX_INFO() << message;
            break;
        case hazkey::frontend::LogLevel::Error:
            FCITX_ERROR() << message;
            break;
    }
}

HazkeyEngine::HazkeyEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) {
          return new HazkeyState(this, &ic);
      }),
      connection_(connectionHooks_, kConnectPolicy),
      client_(connection_, connectionHooks_) {
    connection_.connect();

    instance->inputContextManager().registerProperty("hazkeyState", &factory_);
    reloadConfig();
}

void HazkeyEngine::keyEvent([[maybe_unused]] const InputMethodEntry &entry,
                            KeyEvent &keyEvent) {
    FCITX_DEBUG() << "keyEvent: " << keyEvent.key().toString();

    auto inputContext = keyEvent.inputContext();
    inputContext->propertyFor(&factory_)->keyEvent(keyEvent);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void HazkeyEngine::activate([[maybe_unused]] const InputMethodEntry &entry,
                            InputContextEvent &event) {
    FCITX_DEBUG() << &entry;
    FCITX_DEBUG() << "HazkeyEngine activate";
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyFor(&factory_);
    state->reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void HazkeyEngine::deactivate([[maybe_unused]] const InputMethodEntry &entry,
                              InputContextEvent &event) {
    FCITX_DEBUG() << "HazkeyEngine deactivate";
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyFor(&factory_);
    state->commitAndReset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void HazkeyEngine::setConfig(const RawConfig &config) {
    config_.load(config, true);
    safeSaveAsIni(config_, "conf/hazkey.conf");
    reloadConfig();
}

void HazkeyEngine::reloadConfig() {
    readAsIni(config_, "conf/hazkey.conf");

    std::string lastVersion = config_.lastVersion.value();

    if (lastVersion != HAZKEY_VERSION) {
        FCITX_DEBUG() << "Update detected. restarting server..";
        connectionHooks_.startServer(true);

        config_.lastVersion.setValue(HAZKEY_VERSION);
        safeSaveAsIni(config_, "conf/hazkey.conf");
    }
}

// The `save()` function may be called after a SIGTERM signal is sent during shutdown.
// If you start the hazkey-server at this point, the SIGTERM signal is not sent to the server, causing it to survive until the timeout.
// Therefore, you must not start the hazkey-server here.
void HazkeyEngine::save() {
    client_.saveLearningData(false);
}

FCITX_ADDON_FACTORY(HazkeyEngineFactory);

}  // namespace fcitx
