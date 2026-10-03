#ifndef HAZKEY_FRONTEND_STATE_MACHINE_H_
#define HAZKEY_FRONTEND_STATE_MACHINE_H_

#include <optional>
#include <string>
#include <vector>

#include "hazkey/frontend/frontend_hooks.h"
#include "hazkey/frontend/key_event.h"
#include "hazkey/frontend/keybindings.h"
#include "hazkey/frontend/output.h"
#include "hazkey/frontend/reading_history.h"
#include "hazkey/frontend/server_api.h"

namespace hazkey::frontend {

// Key handling state machine shared by all frontends. One instance per input
// context (fcitx5) or session (emacs). The reading history is shared by all
// instances in the process.
class StateMachine {
   public:
    StateMachine(ServerApi& server, FrontendHooks& hooks,
                 ReadingHistory& history);

    Output keyEvent(const KeyEvent& event);
    // Discard the composition (e.g. on input method activation, focus out
    // or a click in the application). The reading is kept in the history.
    Output reset();
    // Commit the current preedit as is, then reset (e.g. on deactivation).
    Output commitAndReset();

   private:
    enum class ConversionMode {
        Hiragana,
        KatakanaFullwidth,
        KatakanaHalfwidth,
        RawFullwidth,
        RawHalfwidth,
    };

    using CharType = ServerApi::CharType;

    // What the preedit shows while composing: the auto-conversion (the
    // surface), or the reading as typed.
    enum class PreeditView {
        Surface,
        Reading,
    };

    Output finish(KeyResult result);
    // read the key bindings and the history size from the profile
    void applyClientSettings();
    static bool isModifierKey(uint32_t sym);

    // key handlers. return true if consumed, false to pass the key to the
    // application
    bool noPreeditKeyEvent(const KeyEvent& event);
    bool preeditKeyEvent(const KeyEvent& event);
    bool candidateKeyEvent(const KeyEvent& event);
    void composingAction(Action action);
    void candidateAction(Action action);
    // convert_to.* actions, available in both contexts
    void convertToAction(Action action);
    static bool isAltDigitKeyEvent(const KeyEvent& event);
    static int selectionKeyIndex(const KeyEvent& event);

    // convert to hiragana/katakana/alphanumeric directly
    void directCharacterConversion(ConversionMode mode);
    void updateSurroundingText(const std::string& appendText = "");

    // complete the selected candidate and commit it
    void candidateCompleteHandler();
    bool showCandidateList(bool isSuggest);
    void showNonPredictCandidateList(bool preserveTarget = false);
    void showPreeditCandidateList();
    void updateCandidateCursor();
    void advanceCandidateCursor();
    void backCandidateCursor();
    void nextCandidatePage();
    void prevCandidatePage();
    void moveSegmentBoundary(bool expand);

    // cursor and the preedit views
    void cursorAction(Action action);
    // true when the cursor of the reading is at its end
    bool cursorAtEnd();
    // true when the cursor is neither at the start nor at the end: there is
    // a part before it to convert on its own
    bool cursorMidway();
    void enterReadingView();
    // snap the cursor to the start of the element it is in, then show the
    // auto-conversion again
    void leaveReadingView();
    // after an edit in the surface view: the reading is what was edited,
    // so show it while the cursor is not at the end
    void enterReadingViewIfMidway();
    // preedit of the reading view: the reading as one plain segment
    void showReadingPreedit();
    // caret position for the current preedit and view. -1 at the end
    int caretForPreedit(const TextWithCursor& cursor);

    void setCandidateCursorAux();
    void setAuxDown(AuxDown aux);
    void setHiraganaAux();

    void shelveTrailingClause();
    void completePrefixAndCommit();
    void restoreShelvedReadings();
    void clearShelvedReadings();
    // shelved readings in the order they were typed
    std::string shelvedReadingsInOrder() const;

    // Keep the reading of the whole composition in the history. Called
    // before it is committed or discarded. withShelved adds the shelved
    // readings when they are discarded with it.
    void rememberReading(bool withShelved);
    // replace the composition with the reading at the index of the history
    void restoreReading(size_t index);

    // preedit
    void setMultiSegmentPreedit(const std::vector<std::string>& texts,
                                int cursorSegment);
    void setSimplePreeditHighlighted(const std::string& text);
    void setSimplePreedit(const std::string& text);
    void setSimplePreeditWithFurigana(const std::string& text,
                                      int stablePrefixLen,
                                      const std::string& furigana);
    void commitPreedit();
    void commit(const std::string& text);

    // clear preedit, aux and candidates on display
    void resetPanel();
    void resetState();

    ServerApi& server_;
    FrontendHooks& hooks_;
    ReadingHistory& history_;
    KeyBindings keyBindings_;

    PreeditView view_ = PreeditView::Surface;
    // true while the preedit shows the auto-conversion (live text), which
    // is what the cursor moves over by elements
    bool preeditIsLiveText_ = false;
    bool isClauseBoundaryAdjusting_ = false;
    bool isDirectConversionMode_ = false;
    std::optional<CharType> directConversionCharType_;
    int livePreeditIndex_ = -1;
    bool completedWithNoRemaining_ = false;
    std::vector<std::string> shelvedReadings_;
    // index in the history of the reading restored by the last key, while
    // the same key goes back to older ones. -1 otherwise
    int recallIndex_ = -1;

    // text committed when the preedit is committed as is. also sent to the
    // server as the current preedit.
    std::string preeditText_;

    // display state
    Preedit preedit_;
    std::vector<Segment> auxUp_;
    AuxDown auxDown_ = AuxDown::None;
    CandidateWindow candidates_;
    std::string pendingCommit_;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_STATE_MACHINE_H_
