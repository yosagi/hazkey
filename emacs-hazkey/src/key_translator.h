#ifndef HAZKEY_EMACS_KEY_TRANSLATOR_H_
#define HAZKEY_EMACS_KEY_TRANSLATOR_H_

#include <cstdint>
#include <string>
#include <vector>

struct KeyEvent {
    uint32_t keycode = 0;
    std::string key_string;
    bool shift = false;
    bool ctrl = false;
    bool meta = false;

    enum SpecialKey {
        NONE,
        SPACE,
        RETURN,
        BACKSPACE,
        DELETE_KEY,
        TAB,
        ESCAPE,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        F6,
        F7,
        F8,
        F9,
        F10,
        HENKAN,
        MUHENKAN,
    } special = NONE;

    bool isInputable() const;
    std::string inputString() const;
};

namespace KeyTranslator {

KeyEvent translate(const std::vector<std::string>& tokens);

}

#endif
