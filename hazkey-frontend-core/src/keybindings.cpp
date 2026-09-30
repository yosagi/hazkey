#include "hazkey/frontend/keybindings.h"

#include <algorithm>
#include <cctype>

#include "hazkey/frontend/keysyms.h"

namespace hazkey::frontend {

namespace {

struct KeyName {
    const char* name;
    uint32_t sym;
};

// X11 keysym names accepted in addition to single printable characters
constexpr KeyName kKeyNames[] = {
    {"space", keysym::Space},
    {"plus", keysym::Plus},
    {"BackSpace", keysym::BackSpace},
    {"Tab", keysym::Tab},
    {"Return", keysym::Return},
    {"Escape", keysym::Escape},
    {"Delete", keysym::Delete},
    {"Insert", keysym::Insert},
    {"Home", keysym::Home},
    {"End", keysym::End},
    {"Page_Up", keysym::Page_Up},
    {"Page_Down", keysym::Page_Down},
    {"Left", keysym::Left},
    {"Up", keysym::Up},
    {"Right", keysym::Right},
    {"Down", keysym::Down},
    {"Muhenkan", keysym::Muhenkan},
    {"Henkan", keysym::Henkan},
    {"Henkan_Mode", keysym::Henkan},
    {"Hiragana_Katakana", keysym::Hiragana_Katakana},
    {"Zenkaku_Hankaku", keysym::Zenkaku_Hankaku},
    {"Eisu_toggle", keysym::Eisu_toggle},
    {"F1", keysym::F1},
    {"F2", keysym::F2},
    {"F3", keysym::F3},
    {"F4", keysym::F4},
    {"F5", keysym::F5},
    {"F6", keysym::F6},
    {"F7", keysym::F7},
    {"F8", keysym::F8},
    {"F9", keysym::F9},
    {"F10", keysym::F10},
    {"F11", keysym::F11},
    {"F12", keysym::F12},
};

bool equalsIgnoreCase(const std::string& a, const char* b) {
    size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; i++) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return i == a.size() && b[i] == '\0';
}

// letters are compared in lower case; Shift is in the modifiers
uint32_t normalizeSym(uint32_t sym) {
    if (sym >= 'A' && sym <= 'Z') return sym - 'A' + 'a';
    return sym;
}

std::optional<uint32_t> parseKeyName(const std::string& name) {
    if (name.size() == 1) {
        unsigned char c = static_cast<unsigned char>(name[0]);
        if (c > 0x20 && c < 0x7f) {
            // printable ASCII keysyms are the character codes
            return normalizeSym(c);
        }
        return std::nullopt;
    }
    for (const auto& key : kKeyNames) {
        if (equalsIgnoreCase(name, key.name)) return key.sym;
    }
    return std::nullopt;
}

}  // namespace

const std::vector<ActionInfo>& actionTable() {
    using A = Action;
    using C = KeyContext;
    static const std::vector<ActionInfo> table = {
        {A::ComposingCommit, "composing.commit", C::Composing, {"Return"}},
        {A::ComposingCommitPrefix, "composing.commit_prefix", C::Composing,
         {"Shift+Return"}},
        {A::ComposingShelveTrailingClause, "composing.shelve_trailing_clause",
         C::Composing, {"Shift+BackSpace", "Control+Shift+h"}},
        {A::ComposingDeleteLeft, "composing.delete_left", C::Composing,
         {"BackSpace", "Control+h"}},
        {A::ComposingDeleteRight, "composing.delete_right", C::Composing,
         {"Delete"}},
        {A::ComposingCancel, "composing.cancel", C::Composing, {"Escape"}},
        {A::ComposingConvert, "composing.convert", C::Composing,
         {"space", "Henkan"}},
        {A::ComposingInsertSpace, "composing.insert_space", C::Composing,
         {"Shift+space"}},
        {A::ComposingFocusCandidates, "composing.focus_candidates",
         C::Composing, {"Up", "Down", "Tab", "Shift+Tab"}},
        {A::ComposingCursorLeft, "composing.cursor_left", C::Composing,
         {"Left"}},
        {A::ComposingCursorRight, "composing.cursor_right", C::Composing,
         {"Right"}},

        {A::CandidateNext, "candidate.next", C::Candidate,
         {"space", "Tab", "Down"}},
        {A::CandidatePrev, "candidate.prev", C::Candidate,
         {"Shift+space", "Shift+Tab", "Up"}},
        {A::CandidateNextPage, "candidate.next_page", C::Candidate,
         {"Right"}},
        {A::CandidatePrevPage, "candidate.prev_page", C::Candidate, {"Left"}},
        {A::CandidateExpandSegment, "candidate.expand_segment", C::Candidate,
         {"Shift+Right"}},
        {A::CandidateShrinkSegment, "candidate.shrink_segment", C::Candidate,
         {"Shift+Left"}},
        {A::CandidateCommit, "candidate.commit", C::Candidate, {"Return"}},
        {A::CandidateCancel, "candidate.cancel", C::Candidate, {"Escape"}},
        {A::CandidateBack, "candidate.back", C::Candidate,
         {"BackSpace", "Control+h"}},

        {A::ConvertToHiragana, "convert_to.hiragana", C::Any,
         {"F6", "Control+u"}},
        {A::ConvertToKatakanaFull, "convert_to.katakana_full", C::Any,
         {"F7", "Control+i"}},
        {A::ConvertToKatakanaHalf, "convert_to.katakana_half", C::Any,
         {"F8", "Control+o"}},
        {A::ConvertToAlphanumericFull, "convert_to.alphanumeric_full", C::Any,
         {"F9", "Control+p"}},
        {A::ConvertToAlphanumericHalf, "convert_to.alphanumeric_half", C::Any,
         {"F10", "Control+t"}},
    };
    return table;
}

std::optional<KeyChord> parseKeyChord(const std::string& text) {
    KeyChord chord;
    size_t start = 0;
    while (true) {
        size_t plus = text.find('+', start);
        std::string part = text.substr(start, plus == std::string::npos
                                                  ? std::string::npos
                                                  : plus - start);
        if (plus == std::string::npos) {
            auto sym = parseKeyName(part);
            if (!sym.has_value()) return std::nullopt;
            chord.sym = *sym;
            return chord;
        }
        if (equalsIgnoreCase(part, "Control") || equalsIgnoreCase(part, "Ctrl")) {
            chord.mods |= mod::Ctrl;
        } else if (equalsIgnoreCase(part, "Shift")) {
            chord.mods |= mod::Shift;
        } else if (equalsIgnoreCase(part, "Alt")) {
            chord.mods |= mod::Alt;
        } else {
            return std::nullopt;
        }
        start = plus + 1;
    }
}

KeyBindings::KeyBindings() {
    const auto& table = actionTable();
    keys_.resize(table.size());
    for (size_t i = 0; i < table.size(); i++) {
        for (const char* key : table[i].defaultKeys) {
            // the default table is known to parse
            keys_[i].push_back(*parseKeyChord(key));
        }
    }
    rebuild();
}

void KeyBindings::assign(
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        entries,
    std::vector<std::string>& errors) {
    const auto& table = actionTable();
    for (const auto& [id, keys] : entries) {
        auto it = std::find_if(table.begin(), table.end(),
                               [&](const ActionInfo& info) {
                                   return id == info.id;
                               });
        if (it == table.end()) {
            errors.push_back("unknown action: " + id);
            continue;
        }
        auto& chords = keys_[it - table.begin()];
        chords.clear();
        for (const auto& key : keys) {
            auto chord = parseKeyChord(key);
            if (chord.has_value()) {
                chords.push_back(*chord);
            } else {
                errors.push_back("unreadable key for " + id + ": " + key);
            }
        }
    }
    rebuild();
}

void KeyBindings::rebuild() {
    const auto& table = actionTable();
    bindings_.clear();
    for (size_t i = 0; i < table.size(); i++) {
        for (const auto& chord : keys_[i]) {
            bindings_.push_back({chord, table[i].action, table[i].context});
        }
    }
}

std::optional<Action> KeyBindings::find(KeyContext context, uint32_t sym,
                                        uint8_t mods) const {
    for (auto ctx : {context, KeyContext::Any}) {
        for (const auto& b : bindings_) {
            if (b.context == ctx && b.chord.sym == sym &&
                b.chord.mods == mods) {
                return b.action;
            }
        }
    }
    return std::nullopt;
}

std::optional<Action> KeyBindings::lookup(KeyContext context,
                                          const KeyEvent& event) const {
    uint32_t sym = normalizeSym(event.sym);
    if (sym == 0) return std::nullopt;
    return find(context, sym, event.mods);
}

}  // namespace hazkey::frontend
