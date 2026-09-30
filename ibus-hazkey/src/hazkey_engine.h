#ifndef IBUS_HAZKEY_HAZKEY_ENGINE_H_
#define IBUS_HAZKEY_HAZKEY_ENGINE_H_

#include <ibus.h>

namespace hazkey::frontend {
class ServerClient;
}

G_BEGIN_DECLS

#define HAZKEY_TYPE_ENGINE (hazkey_engine_get_type())

GType hazkey_engine_get_type(void);

G_END_DECLS

// The server connection shared by all engine instances (one per input
// context). Set before the factory creates the first engine.
void hazkey_engine_set_server(hazkey::frontend::ServerClient* server);

#endif  // IBUS_HAZKEY_HAZKEY_ENGINE_H_
