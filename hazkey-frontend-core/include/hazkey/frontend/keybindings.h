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
    CandidateDeleteLeft,
    CandidateIgnore,

    ConvertToHiragana,
    ConvertToKatakanaFull,
    ConvertToKatakanaHalf,
    ConvertToAlphanumericFull,
    ConvertToAlphanumericHalf,
};

// Where an action applies.
enum class KeyContext {
    // preedit is shown and the candidate list is not focused
    Composing,
    // the candidate list is focused
    Candidate,
    // both of the above
    Any,
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
};

// Parse a key like "Control+Shift+h": modifiers (Control, Shift, Alt) and
// an X11 keysym name joined with "+". Names are case insensitive.
std::optional<KeyChord> parseKeyChord(const std::string& text);

// Keys assigned to the actions.
//
// A key matches a binding when the keysym and the modifiers are the same.
// Letters match regardless of case, so the modifiers alone decide. If no
// binding matches exactly, a binding without modifiers also matches the key
// with modifiers held (Control+Return works as Return), except for letters
// and digits.
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
