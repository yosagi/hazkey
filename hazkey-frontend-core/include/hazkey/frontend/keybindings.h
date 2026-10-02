#ifndef HAZKEY_FRONTEND_KEYBINDINGS_H_
#define HAZKEY_FRONTEND_KEYBINDINGS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "hazkey/frontend/key_event.h"

namespace hazkey::frontend {

// Operations the state machine performs on key presses. The key assigned to
// each one comes from KeyBindings.
enum class Action {
    ComposingCommit,
    ComposingCommitPrefix,
    ComposingShelveTrailingClause,
    ComposingDeleteLeft,
    ComposingDeleteRight,
    ComposingCancel,
    ComposingConvert,
    ComposingInsertSpace,
    ComposingFocusCandidates,
    ComposingCursorLeft,
    ComposingCursorRight,

    CandidateNext,
    CandidatePrev,
    CandidateNextPage,
    CandidatePrevPage,
    CandidateExpandSegment,
    CandidateShrinkSegment,
    CandidateCommit,
    CandidateCancel,
    CandidateBack,

    ConvertToHiragana,
    ConvertToKatakanaFull,
    ConvertToKatakanaHalf,
    ConvertToAlphanumericFull,
    ConvertToAlphanumericHalf,

    IdleRestoreReading,
};

// Where an action applies.
enum class KeyContext {
    // preedit is shown and the candidate list is not focused
    Composing,
    // the candidate list is focused
    Candidate,
    // both of the above
    Any,
    // no preedit. not included in Any
    Idle,
};

struct ActionInfo {
    Action action;
    // name in the config, e.g. "composing.commit"
    const char* id;
    KeyContext context;
    std::vector<const char*> defaultKeys;
};

// All actions with their default keys, in lookup order.
const std::vector<ActionInfo>& actionTable();

struct KeyChord {
    uint32_t sym = 0;
    uint8_t mods = mod::None;

    bool operator==(const KeyChord& other) const {
        return sym == other.sym && mods == other.mods;
    }
    bool operator!=(const KeyChord& other) const { return !(*this == other); }
};

// Parse a key like "Control+Shift+h": modifiers (Control, Shift, Alt) and
// an X11 keysym name joined with "+". Names are case insensitive.
std::optional<KeyChord> parseKeyChord(const std::string& text);

// The inverse of parseKeyChord: the canonical text of a key, with the
// modifiers in the order Control, Shift, Alt and letters in lower case.
// Empty for a keysym that has no name here.
std::optional<std::string> formatKeyChord(const KeyChord& chord);

// Keys assigned to the actions.
//
// A key matches a binding when the keysym and the modifiers are the same.
// Letters match regardless of case, so the modifiers alone decide. Keys with
// other modifiers held do not match; list them as keys of their own.
class KeyBindings {
   public:
    // the default bindings
    KeyBindings();

    // Replace the keys of the given actions. Actions not listed keep the
    // default keys; an action listed with no keys is left unbound.
    // Unknown action ids and unreadable keys are skipped and reported in
    // errors.
    void assign(const std::vector<std::pair<std::string,
                                            std::vector<std::string>>>& entries,
                std::vector<std::string>& errors);

    std::optional<Action> lookup(KeyContext context,
                                 const KeyEvent& event) const;

   private:
    struct Binding {
        KeyChord chord;
        Action action;
        KeyContext context;
    };

    // keys of each action, indexed like actionTable()
    std::vector<std::vector<KeyChord>> keys_;
    std::vector<Binding> bindings_;

    void rebuild();
    std::optional<Action> find(KeyContext context, uint32_t sym,
                               uint8_t mods) const;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_KEYBINDINGS_H_
