#include "key_translator.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "hazkey/frontend/keysyms.h"
#include "mozc_emacs_helper_lib.h"

namespace keysym = hazkey::frontend::keysym;
namespace mod = hazkey::frontend::mod;

namespace {

// mozc.el special key names
const std::unordered_map<std::string, uint32_t> kSpecialKeys = {
    {"space", keysym::Space},     {"return", keysym::Return},
    {"backspace", keysym::BackSpace}, {"delete", keysym::Delete},
    {"tab", keysym::Tab},         {"escape", keysym::Escape},
    {"up", keysym::Up},           {"down", keysym::Down},
    {"left", keysym::Left},       {"right", keysym::Right},
    {"home", keysym::Home},       {"end", keysym::End},
    {"pageup", keysym::Page_Up},  {"pagedown", keysym::Page_Down},
    {"insert", keysym::Insert},
    {"f1", keysym::F1},           {"f2", keysym::F2},
    {"f3", keysym::F3},           {"f4", keysym::F4},
    {"f5", keysym::F5},           {"f6", keysym::F6},
    {"f7", keysym::F7},           {"f8", keysym::F8},
    {"f9", keysym::F9},           {"f10", keysym::F10},
    {"f11", keysym::F11},         {"f12", keysym::F12},
    {"henkan", keysym::Henkan},   {"muhenkan", keysym::Muhenkan},
    {"kana", keysym::Hiragana_Katakana},
    {"zenkaku-hankaku", keysym::Zenkaku_Hankaku},
    {"eisu", keysym::Eisu_toggle},
};

bool isPrintableAscii(uint32_t keycode) {
    return keycode >= 0x20 && keycode < 0x7f;
}

}  // namespace

hazkey::frontend::KeyEvent KeyTranslator::translate(
    const std::vector<std::string>& tokens) {
    hazkey::frontend::KeyEvent event;
    uint32_t keycode = 0;
    uint32_t special = 0;
    std::string keyString;

    for (const auto& tok : tokens) {
        if (tok.empty()) continue;

        if (tok[0] == '"') {
            mozc::emacs::UnquoteString(tok, &keyString);
        } else if (std::isdigit(static_cast<unsigned char>(tok[0]))) {
            keycode =
                static_cast<uint32_t>(std::strtoul(tok.c_str(), nullptr, 10));
        } else if (tok == "shift") {
            event.mods |= mod::Shift;
        } else if (tok == "control") {
            event.mods |= mod::Ctrl;
        } else if (tok == "meta" || tok == "alt") {
            event.mods |= mod::Alt;
        } else if (auto it = kSpecialKeys.find(tok); it != kSpecialKeys.end()) {
            special = it->second;
        }
    }

    // printable ASCII keycodes are the same as their keysyms
    if (special != 0) {
        event.sym = special;
    } else if (isPrintableAscii(keycode)) {
        event.sym = keycode;
    }

    if (!(event.mods & (mod::Ctrl | mod::Alt))) {
        if (special == keysym::Space) {
            event.text = " ";
        } else if (!keyString.empty()) {
            event.text = keyString;
        } else if (special == 0 && isPrintableAscii(keycode)) {
            event.text = std::string(1, static_cast<char>(keycode));
        }
    }

    return event;
}
