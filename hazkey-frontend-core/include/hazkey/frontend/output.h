#ifndef HAZKEY_FRONTEND_OUTPUT_H_
#define HAZKEY_FRONTEND_OUTPUT_H_

#include <string>
#include <vector>

namespace hazkey::frontend {

enum class SegmentStyle {
    Normal,
    Underline,
    Highlight,
};

struct Segment {
    std::string text;
    SegmentStyle style = SegmentStyle::Normal;
};

struct Preedit {
    std::vector<Segment> segments;
    // Index of the segment whose start holds the caret. -1 if unspecified.
    int caretSegment = -1;
};

struct Candidate {
    std::string text;
    // Remaining reading not covered by this candidate (partial conversion).
    std::string subHiragana;
};

struct CandidateWindow {
    bool visible = false;
    std::vector<Candidate> items;
    // Global index of the selected candidate. -1 if the window is not focused.
    int cursor = -1;
    int pageSize = 0;
    int page = 0;
    // true for conversion (Space), false for prediction while typing.
    bool isConversion = false;

    bool focused() const { return cursor >= 0; }
    int pageStart() const { return page * pageSize; }
    int pageCount() const;
    int pageItemCount() const;
};

enum class KeyResult {
    // not handled. other key handlers of the frontend may process it.
    Ignored,
    // not handled. send it to the application as is.
    PassToApplication,
    // handled by the input method.
    Consumed,
};

enum class AuxDown {
    None,
    TabToSelect,
    DirectInput,
};

// Full UI state after handling one event. Frontends replace their current
// display with this.
struct Output {
    KeyResult result = KeyResult::Ignored;
    // Text to commit before showing the preedit. Concatenated when the event
    // commits more than once.
    std::string commit;
    Preedit preedit;
    // Upper auxiliary text: the reading with cursor, or the candidate
    // position like "[1/20]".
    std::vector<Segment> auxUp;
    // Lower auxiliary text. Frontends translate it.
    AuxDown auxDown = AuxDown::None;
    CandidateWindow candidates;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_OUTPUT_H_
