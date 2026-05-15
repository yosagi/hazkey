#include "key_translator.h"

#include <cctype>
#include <cstdlib>
#include <string>

#include "mozc_emacs_helper_lib.h"

bool KeyEvent::isInputable() const {
    if (ctrl || meta) return false;
    if (special == SPACE) return true;
    if (!key_string.empty()) return true;
    if (keycode >= 0x20 && keycode < 0x7f) return true;
    return false;
}

std::string KeyEvent::inputString() const {
    if (!key_string.empty()) return key_string;
    if (special == SPACE) return " ";
    if (keycode >= 0x20 && keycode < 0x7f) {
        return std::string(1, static_cast<char>(keycode));
    }
    return "";
}

KeyEvent KeyTranslator::translate(const std::vector<std::string>& tokens) {
    KeyEvent event;

    for (const auto& tok : tokens) {
        if (tok.empty()) continue;

        if (tok[0] == '"') {
            mozc::emacs::UnquoteString(tok, &event.key_string);
        } else if (std::isdigit(static_cast<unsigned char>(tok[0]))) {
            event.keycode = static_cast<uint32_t>(std::strtoul(tok.c_str(), nullptr, 10));
        } else if (tok == "shift") {
            event.shift = true;
        } else if (tok == "control") {
            event.ctrl = true;
        } else if (tok == "meta" || tok == "alt") {
            event.meta = true;
        } else if (tok == "space") {
            event.special = KeyEvent::SPACE;
        } else if (tok == "return") {
            event.special = KeyEvent::RETURN;
        } else if (tok == "backspace") {
            event.special = KeyEvent::BACKSPACE;
        } else if (tok == "delete") {
            event.special = KeyEvent::DELETE_KEY;
        } else if (tok == "tab") {
            event.special = KeyEvent::TAB;
        } else if (tok == "escape") {
            event.special = KeyEvent::ESCAPE;
        } else if (tok == "up") {
            event.special = KeyEvent::UP;
        } else if (tok == "down") {
            event.special = KeyEvent::DOWN;
        } else if (tok == "left") {
            event.special = KeyEvent::LEFT;
        } else if (tok == "right") {
            event.special = KeyEvent::RIGHT;
        } else if (tok == "f6") {
            event.special = KeyEvent::F6;
        } else if (tok == "f7") {
            event.special = KeyEvent::F7;
        } else if (tok == "f8") {
            event.special = KeyEvent::F8;
        } else if (tok == "f9") {
            event.special = KeyEvent::F9;
        } else if (tok == "f10") {
            event.special = KeyEvent::F10;
        } else if (tok == "henkan") {
            event.special = KeyEvent::HENKAN;
        } else if (tok == "muhenkan") {
            event.special = KeyEvent::MUHENKAN;
        }
    }

    return event;
}
