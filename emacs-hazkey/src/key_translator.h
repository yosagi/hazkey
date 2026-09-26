#ifndef HAZKEY_EMACS_KEY_TRANSLATOR_H_
#define HAZKEY_EMACS_KEY_TRANSLATOR_H_

#include <string>
#include <vector>

#include "hazkey/frontend/key_event.h"

namespace KeyTranslator {

// Convert the key tokens of a mozc.el SendKey command (e.g. `97`, `"あ"`,
// `return`, `control`) to a key event of the shared state machine.
hazkey::frontend::KeyEvent translate(const std::vector<std::string>& tokens);

}  // namespace KeyTranslator

#endif
