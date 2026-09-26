#ifndef HAZKEY_FRONTEND_STATE_MACHINE_H_
#define HAZKEY_FRONTEND_STATE_MACHINE_H_

#include <optional>
#include <string>
#include <vector>

#include "hazkey/frontend/frontend_hooks.h"
#include "hazkey/frontend/key_event.h"
#include "hazkey/frontend/output.h"
#include "hazkey/frontend/server_api.h"

namespace hazkey::frontend {

// Key handling state machine shared by all frontends. One instance per input
// context (fcitx5) or session (emacs).
class StateMachine {
   public:
    StateMachine(ServerApi& server, FrontendHooks& hooks);

    Output keyEvent(const KeyEvent& event);
    // Discard the composition (e.g. on input method activation).
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

    Output finish(KeyResult result);

    // key handlers. return true if consumed, false to pass the key to the
    // application
    bool noPreeditKeyEvent(const KeyEvent& event);
    bool preeditKeyEvent(const KeyEvent& event);
    bool candidateKeyEvent(const KeyEvent& event);
    bool ctrlShortcutHandler(uint32_t sym);
    // f6-f10 key handler
    void functionKeyHandler(uint32_t sym);
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

    void setCandidateCursorAux();
    void setAuxDown(AuxDown aux);
    void setHiraganaAux();

    void shelveTrailingClause();
    void completePrefixAndCommit();
    void restoreShelvedReadings();
    void clearShelvedReadings();

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

    bool isCursorMoving_ = false;
    bool isClauseBoundaryAdjusting_ = false;
    bool isDirectConversionMode_ = false;
    std::optional<CharType> directConversionCharType_;
    int livePreeditIndex_ = -1;
    bool completedWithNoRemaining_ = false;
    std::vector<std::string> shelvedReadings_;
    std::string lastTrailingClauseYomi_;

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
