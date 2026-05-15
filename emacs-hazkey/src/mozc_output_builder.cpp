#include "mozc_output_builder.h"

#include <string>

#include "mozc_emacs_helper_lib.h"

using mozc::emacs::QuoteString;

static std::string buildPreedit(const OutputData& output) {
    if (output.preedit_segments.empty()) return "";

    std::string result = "(preedit . ((cursor . ";
    result += std::to_string(output.preedit_cursor);
    result += ")(segment ";
    for (const auto& seg : output.preedit_segments) {
        result += "(";
        if (seg.highlight) {
            result += "(annotation . highlight)";
        } else {
            result += "(annotation . underline)";
        }
        result += "(value . ";
        result += QuoteString(seg.value);
        result += ")";
        // value-length: character count for cursor positioning
        int charCount = 0;
        for (size_t i = 0; i < seg.value.size();) {
            unsigned char ch = seg.value[i];
            if (ch < 0x80)
                i += 1;
            else if (ch < 0xE0)
                i += 2;
            else if (ch < 0xF0)
                i += 3;
            else
                i += 4;
            charCount++;
        }
        result += "(value-length . ";
        result += std::to_string(charCount);
        result += ")";
        result += ")";
    }
    result += ")))";
    return result;
}

static std::string buildCandidateWindow(const OutputData& output) {
    if (output.visible_candidates.empty()) return "";

    std::string result = "(candidates . (";
    if (output.candidate_focused_index >= 0) {
        result += "(focused-index . ";
        result += std::to_string(output.candidate_focused_index);
        result += ")";
    }
    result += "(size . ";
    result += std::to_string(output.candidate_total_size);
    result += ")";
    result += "(category . ";
    result += output.candidate_is_conversion ? "conversion" : "suggestion";
    result += ")";
    result += "(footer . ((index-visible . ";
    result += output.candidate_is_conversion ? "t" : "nil";
    result += ")))";

    result += "(candidate ";
    for (const auto& cand : output.visible_candidates) {
        result += "((index . ";
        result += std::to_string(cand.index);
        result += ")(value . ";
        result += QuoteString(cand.text);
        result += ")";
        if (!cand.shortcut.empty()) {
            result += "(annotation . ((shortcut . ";
            result += QuoteString(cand.shortcut);
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
                                              const OutputData& output) {
    std::string result = "((emacs-event-id . ";
    result += std::to_string(eventId);
    result += ")(emacs-session-id . ";
    result += std::to_string(sessionId);
    result += ")(output . (";

    result += "(consumed . ";
    result += output.consumed ? "t" : "nil";
    result += ")";

    if (output.committed_text.has_value()) {
        result += "(result . ((type . string)(value . ";
        result += QuoteString(output.committed_text.value());
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
