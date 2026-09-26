#ifndef HAZKEY_FRONTEND_KEY_EVENT_H_
#define HAZKEY_FRONTEND_KEY_EVENT_H_

#include <cstdint>
#include <string>

namespace hazkey::frontend {

namespace mod {
constexpr uint8_t None = 0;
constexpr uint8_t Shift = 1 << 0;
constexpr uint8_t Ctrl = 1 << 1;
constexpr uint8_t Alt = 1 << 2;
}  // namespace mod

struct KeyEvent {
    // X11 keysym (see keysyms.h). 0 if the frontend only knows the text.
    uint32_t sym = 0;
    // Modifiers physically held (mod::Shift | mod::Ctrl | mod::Alt).
    uint8_t mods = mod::None;
    // UTF-8 text to insert when the frontend treats this key as character
    // input. Empty for keys that do not insert text.
    std::string text;
    bool isRelease = false;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_KEY_EVENT_H_
