#include "mozc_output_builder.h"

#include <string>

#include "mozc_emacs_helper_lib.h"

using mozc::emacs::QuoteString;

using hazkey::frontend::Output;
using hazkey::frontend::SegmentStyle;

static const std::string kSelectionKeys = "1234567890";

static int charCount(const std::string& text) {
    int count = 0;
    for (size_t i = 0; i < text.size();) {
        unsigned char ch = text[i];
        if (ch < 0x80)
            i += 1;
        else if (ch < 0xE0)
            i += 2;
        else if (ch < 0xF0)
            i += 3;
        else
            i += 4;
        count++;
    }
    return count;
}

static std::string buildPreedit(const Output& output) {
    std::string segments;
    for (const auto& seg : output.preedit.segments) {
        if (seg.text.empty()) continue;
        segments += "(";
        if (seg.style == SegmentStyle::Highlight) {
            segments += "(annotation . highlight)";
        } else {
            segments += "(annotation . underline)";
        }
        segments += "(value . ";
        segments += QuoteString(seg.text);
        segments += ")";
        // value-length: character count for cursor positioning
        segments += "(value-length . ";
        segments += std::to_string(charCount(seg.text));
        segments += ")";
        segments += ")";
    }
    if (segments.empty()) return "";

    return "(preedit . ((cursor . 0)(segment " + segments + ")))";
}

static std::string buildCandidateWindow(const Output& output) {
    const auto& window = output.candidates;
    if (!window.visible || window.items.empty()) return "";

    std::string result = "(candidates . (";
    if (window.focused()) {
        result += "(focused-index . ";
        result += std::to_string(window.cursor);
        result += ")";
    }
    result += "(size . ";
    result += std::to_string(window.items.size());
    result += ")";
    result += "(category . ";
    result += window.isConversion ? "conversion" : "suggestion";
    result += ")";
    result += "(footer . ((index-visible . ";
    result += window.isConversion ? "t" : "nil";
    result += ")))";

    result += "(candidate ";
    int pageStart = window.pageStart();
    int pageEnd = pageStart + window.pageItemCount();
    for (int i = pageStart; i < pageEnd; i++) {
        int localIdx = i - pageStart;
        result += "((index . ";
        result += std::to_string(i);
        result += ")(value . ";
        result += QuoteString(window.items[i].text);
        result += ")";
        if (localIdx < static_cast<int>(kSelectionKeys.size())) {
            result += "(annotation . ((shortcut . ";
            result += QuoteString(std::string(1, kSelectionKeys[localIdx]));
            result += ")))";
        }
        result += ")";
    }
    result += ")";

    result += "))";
    return result;
}

std::string MozcOutputBuilder::buildGreeting() {
    return "((mozc-emacs-helper . t)(version . \"0.1.0\")"
           "(config . ((preedit-method . roman))))";
}

std::string MozcOutputBuilder::buildResponse(uint32_t eventId,
                                              uint32_t sessionId,
                                              const Output& output) {
    std::string result = "((emacs-event-id . ";
    result += std::to_string(eventId);
    result += ")(emacs-session-id . ";
    result += std::to_string(sessionId);
    result += ")(output . (";

    result += "(consumed . ";
    result += output.result == hazkey::frontend::KeyResult::Consumed ? "t"
                                                                      : "nil";
    result += ")";

    if (!output.commit.empty()) {
        result += "(result . ((type . string)(value . ";
        result += QuoteString(output.commit);
        result += ")))";
    }

    std::string preedit = buildPreedit(output);
    if (!preedit.empty()) result += preedit;

    std::string candidates = buildCandidateWindow(output);
    if (!candidates.empty()) result += candidates;

    result += ")))";
    return result;
}

std::string MozcOutputBuilder::buildCreateSessionResponse(uint32_t eventId,
                                                           uint32_t sessionId) {
    std::string result = "((emacs-event-id . ";
    result += std::to_string(eventId);
    result += ")(emacs-session-id . ";
    result += std::to_string(sessionId);
    result += ")(output . nil))";
    return result;
}

std::string MozcOutputBuilder::buildDeleteSessionResponse(uint32_t eventId,
                                                           uint32_t sessionId) {
    return buildCreateSessionResponse(eventId, sessionId);
}
