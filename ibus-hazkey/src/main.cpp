// ibus engine process. Started by ibus-daemon with --ibus through the
// component file; started by hand without it, it registers the component
// itself (for trying a build without installing it).

#include <ibus.h>

#include <cstring>
#include <string>

#include "hazkey/frontend/server_client.h"
#include "hazkey/frontend/server_connection.h"
#include "hazkey_engine.h"

namespace {

namespace hf = hazkey::frontend;

constexpr char kComponentName[] = "org.freedesktop.IBus.Hazkey";

// Logs go to the log of ibus-daemon (stderr of this process).
class IbusConnectionHooks : public hf::ConnectionHooks {
   public:
    void startServer(bool forceRestart) override {
        g_message("starting hazkey-server");
        hf::spawnServerDetached(forceRestart);
    }
    void log(hf::LogLevel level, const std::string& message) override {
        switch (level) {
            case hf::LogLevel::Debug:
                g_debug("%s", message.c_str());
                break;
            case hf::LogLevel::Info:
                g_message("%s", message.c_str());
                break;
            case hf::LogLevel::Error:
                g_warning("%s", message.c_str());
                break;
        }
    }
};

// Start the server after the 1st failed attempt. No forced restart (-r): it
// would kill the server other clients are using, or one that is still
// loading its dictionary. Same as fcitx5-hazkey.
constexpr hf::ConnectPolicy kConnectPolicy{
    /*maxRetries=*/8, /*retryIntervalMs=*/250, /*startAttempt=*/0,
    /*forceRestartAttempt=*/-1};

void onDisconnected([[maybe_unused]] IBusBus* bus, gpointer data) {
    // ibus-daemon has gone (e.g. the session ends). do not start the server
    // here, it would outlive the session.
    static_cast<hf::ServerClient*>(data)->saveLearningData(false);
    ibus_quit();
}

void registerComponent(IBusBus* bus) {
    IBusComponent* component = ibus_component_new(
        kComponentName, "Hazkey Component", HAZKEY_VERSION, "MIT",
        "Hazkey contributors", "https://github.com/7ka-Hiira/hazkey",
        HAZKEY_IBUS_ENGINE_PATH " --ibus", "ibus-hazkey");
    ibus_component_add_engine(
        component,
        ibus_engine_desc_new_varargs(
            "name", "hazkey", "longname", "Hazkey", "description",
            "Japanese input method using AzooKeyKanaKanjiConverter",
            "language", "ja", "license", "MIT", "author",
            "Hazkey contributors", "icon", HAZKEY_ICON_PATH, "layout",
            "default", "symbol", "あ", "setup", HAZKEY_SETTINGS_PATH,
            nullptr));
    ibus_bus_register_component(bus, component);
}

}  // namespace

int main(int argc, char** argv) {
    bool startedByIbus = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--ibus") == 0 ||
            std::strcmp(argv[i], "-i") == 0) {
            startedByIbus = true;
        }
    }

    ibus_init();
    IBusBus* bus = ibus_bus_new();
    if (!ibus_bus_is_connected(bus)) {
        g_warning("cannot connect to ibus-daemon");
        return 1;
    }

    static IbusConnectionHooks hooks;
    static hf::ServerConnection connection(hooks, kConnectPolicy);
    static hf::ServerClient client(connection, hooks);
    connection.connect();
    hazkey_engine_set_server(&client);

    g_signal_connect(bus, "disconnected", G_CALLBACK(onDisconnected), &client);

    IBusFactory* factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "hazkey", HAZKEY_TYPE_ENGINE);

    if (startedByIbus) {
        ibus_bus_request_name(bus, kComponentName, 0);
    } else {
        registerComponent(bus);
    }

    ibus_main();
    return 0;
}
