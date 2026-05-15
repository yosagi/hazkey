#ifndef HAZKEY_EMACS_STATE_H_
#define HAZKEY_EMACS_STATE_H_

#include <optional>
#include <string>
#include <vector>

#include "hazkey_emacs_connector.h"
#include "key_translator.h"

struct PreeditSegment {
    std::string value;
    bool highlight = false;
};

struct CandidateInfo {
    std::string text;
    int index;
    std::string shortcut;
};

struct OutputData {
    bool consumed = false;

    std::optional<std::string> committed_text;

    std::vector<PreeditSegment> preedit_segments;
    int preedit_cursor = 0;

    std::vector<CandidateInfo> visible_candidates;
    int candidate_focused_index = -1;
    int candidate_total_size = 0;
    bool candidate_is_conversion = false;
};

class HazkeyEmacsState {
   public:
    HazkeyEmacsState(HazkeyEmacsConnector& connector);

    OutputData processKeyEvent(const KeyEvent& event);

   private:
    enum class ConversionMode {
        Hiragana,
        KatakanaFullwidth,
        KatakanaHalfwidth,
        RawFullwidth,
        RawHalfwidth,
    };

    struct CandidateEntry {
        std::string text;
        std::string hiragana;
    };

    OutputData noPreeditKeyEvent(const KeyEvent& event);
    OutputData preeditKeyEvent(const KeyEvent& event);
    OutputData candidateKeyEvent(const KeyEvent& event);

    void reset();
    bool fetchCandidateList(bool isSuggest);
    void showNonPredictCandidateList(bool preserveTarget = false);
    bool showPreeditCandidateList();
    void directCharacterConversion(ConversionMode mode);
    bool ctrlShortcutHandler(const KeyEvent& event);
    void functionKeyHandler(const KeyEvent& event);

    void advanceCandidateCursor();
    void backCandidateCursor();
    void moveSegmentBoundary(bool expand);

    OutputData buildCurrentOutput(bool consumed);
    OutputData buildCommitOutput(const std::string& text);
    OutputData buildNotConsumedOutput();
    void updatePreeditFromCandidate();

    bool isCursorMoving_ = false;
    bool isClauseBoundaryAdjusting_ = false;
    bool isDirectConversionMode_ = false;
    int livePreeditIndex_ = -1;

    std::string commitText_;
    std::vector<PreeditSegment> preeditSegments_;
    int preeditCursor_ = 0;

    std::vector<CandidateEntry> candidates_;
    int candidateCursorIndex_ = -1;
    int candidatePageSize_ = 9;
    bool candidateFocused_ = false;
    bool candidateIsConversion_ = false;

    std::string lastCommittedText_;
    HazkeyEmacsConnector& connector_;
};

#endif
