#include "hazkey_emacs_state.h"

#include <algorithm>

static const std::string SELECTION_KEYS = "1234567890";

HazkeyEmacsState::HazkeyEmacsState(HazkeyEmacsConnector& connector)
    : connector_(connector) {
    connector_.newComposingText();
}

OutputData HazkeyEmacsState::processKeyEvent(const KeyEvent& event) {
    std::string composingText = connector_.getComposingText(
        hazkey::commands::GetComposingString_CharType_HIRAGANA, commitText_);

    if (candidateFocused_ && !candidates_.empty()) {
        return candidateKeyEvent(event);
    } else if (!composingText.empty()) {
        return preeditKeyEvent(event);
    } else {
        return noPreeditKeyEvent(event);
    }
}

OutputData HazkeyEmacsState::noPreeditKeyEvent(const KeyEvent& event) {
    if (event.special == KeyEvent::SPACE) {
        if (event.shift) {
            auto out = buildCommitOutput(" ");
            return out;
        } else {
            connector_.inputChar(" ");
            std::string text = connector_.getComposingText(
                hazkey::commands::GetComposingString_CharType_HIRAGANA, "");
            auto out = buildCommitOutput(text);
            return out;
        }
    }

    if (event.isInputable()) {
        connector_.setContext(lastCommittedText_,
                             lastCommittedText_.length());
        connector_.inputChar(event.inputString());
        showPreeditCandidateList();
        return buildCurrentOutput(true);
    }

    reset();
    return buildNotConsumedOutput();
}

OutputData HazkeyEmacsState::preeditKeyEvent(const KeyEvent& event) {
    switch (event.special) {
        case KeyEvent::RETURN:
            if (livePreeditIndex_ >= 0) {
                connector_.completePrefix(livePreeditIndex_);
            } else if (directConversionCharType_.has_value()) {
                connector_.directConversionComplete(
                    directConversionCharType_.value());
            }
            {
                auto out = buildCommitOutput(commitText_);
                return out;
            }
        case KeyEvent::BACKSPACE:
            connector_.deleteLeft();
            showPreeditCandidateList();
            return buildCurrentOutput(true);
        case KeyEvent::DELETE_KEY:
            connector_.deleteRight();
            showPreeditCandidateList();
            return buildCurrentOutput(true);
        case KeyEvent::ESCAPE:
            reset();
            return buildCurrentOutput(true);
        case KeyEvent::SPACE:
            if (!isDirectConversionMode_ && event.shift) {
                connector_.inputChar(" ");
                showPreeditCandidateList();
            } else {
                showNonPredictCandidateList();
            }
            return buildCurrentOutput(true);
        case KeyEvent::HENKAN:
            showNonPredictCandidateList();
            return buildCurrentOutput(true);
        case KeyEvent::UP:
        case KeyEvent::DOWN:
        case KeyEvent::TAB:
            if (candidates_.empty()) {
                showNonPredictCandidateList();
            } else {
                candidateFocused_ = true;
                candidateCursorIndex_ = 0;
                updatePreeditFromCandidate();
            }
            return buildCurrentOutput(true);
        case KeyEvent::LEFT:
            isCursorMoving_ = true;
            connector_.moveCursor(-1);
            return buildCurrentOutput(true);
        case KeyEvent::RIGHT:
            if (isCursorMoving_) {
                connector_.moveCursor(1);
            }
            return buildCurrentOutput(true);
        case KeyEvent::F6:
        case KeyEvent::F7:
        case KeyEvent::F8:
        case KeyEvent::F9:
        case KeyEvent::F10:
            functionKeyHandler(event);
            return buildCurrentOutput(true);
        default:
            break;
    }

    if (event.ctrl) {
        if (ctrlShortcutHandler(event)) {
            return buildCurrentOutput(true);
        }
        return buildNotConsumedOutput();
    }

    if (event.isInputable()) {
        if (isDirectConversionMode_) {
            lastCommittedText_ += commitText_;
            auto out = buildCommitOutput(commitText_);
            connector_.inputChar(event.inputString());
            showPreeditCandidateList();
            auto current = buildCurrentOutput(true);
            out.preedit_segments = current.preedit_segments;
            out.preedit_cursor = current.preedit_cursor;
            out.visible_candidates = current.visible_candidates;
            out.candidate_total_size = current.candidate_total_size;
            out.candidate_focused_index = current.candidate_focused_index;
            return out;
        }
        connector_.inputChar(event.inputString());
        showPreeditCandidateList();
        return buildCurrentOutput(true);
    }

    return buildNotConsumedOutput();
}

OutputData HazkeyEmacsState::candidateKeyEvent(const KeyEvent& event) {
    switch (event.special) {
        case KeyEvent::RETURN: {
            int globalIndex = candidateCursorIndex_;
            std::string text = candidates_[globalIndex].text;
            bool hasRemaining =
                !candidates_[globalIndex].hiragana.empty();
            lastCommittedText_ += text;
            connector_.completePrefix(globalIndex);
            if (hasRemaining) {
                // Partial completion: commit this part, show candidates
                // for remaining text
                commitText_.clear();
                preeditSegments_.clear();
                candidates_.clear();
                candidateFocused_ = false;
                candidateCursorIndex_ = -1;
                isClauseBoundaryAdjusting_ = false;
                showNonPredictCandidateList(false);
                auto out = buildCurrentOutput(true);
                out.committed_text = text;
                return out;
            }
            return buildCommitOutput(text);
        }
        case KeyEvent::ESCAPE:
            if (isClauseBoundaryAdjusting_) {
                showNonPredictCandidateList(false);
                return buildCurrentOutput(true);
            }
            isClauseBoundaryAdjusting_ = false;
            showPreeditCandidateList();
            return buildCurrentOutput(true);
        case KeyEvent::BACKSPACE:
            showPreeditCandidateList();
            return buildCurrentOutput(true);
        case KeyEvent::SPACE:
        case KeyEvent::TAB:
            if (event.shift) {
                backCandidateCursor();
            } else {
                advanceCandidateCursor();
            }
            return buildCurrentOutput(true);
        case KeyEvent::DOWN:
            advanceCandidateCursor();
            return buildCurrentOutput(true);
        case KeyEvent::UP:
            backCandidateCursor();
            return buildCurrentOutput(true);
        case KeyEvent::RIGHT:
            if (event.shift) {
                moveSegmentBoundary(true);
            } else {
                // next page
                int nextStart =
                    ((candidateCursorIndex_ / candidatePageSize_) + 1) *
                    candidatePageSize_;
                if (nextStart < (int)candidates_.size()) {
                    candidateCursorIndex_ = nextStart;
                    updatePreeditFromCandidate();
                }
            }
            return buildCurrentOutput(true);
        case KeyEvent::LEFT:
            if (event.shift) {
                moveSegmentBoundary(false);
            } else {
                // prev page
                int prevStart =
                    ((candidateCursorIndex_ / candidatePageSize_) - 1) *
                    candidatePageSize_;
                if (prevStart >= 0) {
                    candidateCursorIndex_ = prevStart;
                    updatePreeditFromCandidate();
                }
            }
            return buildCurrentOutput(true);
        case KeyEvent::F6:
        case KeyEvent::F7:
        case KeyEvent::F8:
        case KeyEvent::F9:
        case KeyEvent::F10:
            functionKeyHandler(event);
            return buildCurrentOutput(true);
        default:
            break;
    }

    if (event.ctrl) {
        if (ctrlShortcutHandler(event)) {
            return buildCurrentOutput(true);
        }
        return buildNotConsumedOutput();
    }

    // Digit key selection (1-9, 0)
    if (event.keycode >= '0' && event.keycode <= '9' && !event.ctrl &&
        !event.meta) {
        int localIndex =
            (event.keycode == '0') ? 9 : (event.keycode - '1');
        int pageStart =
            (candidateCursorIndex_ / candidatePageSize_) * candidatePageSize_;
        int globalIndex = pageStart + localIndex;
        if (globalIndex < (int)candidates_.size()) {
            std::string text = candidates_[globalIndex].text;
            bool hasRemaining =
                !candidates_[globalIndex].hiragana.empty();
            lastCommittedText_ += text;
            connector_.completePrefix(globalIndex);
            if (hasRemaining) {
                commitText_.clear();
                preeditSegments_.clear();
                candidates_.clear();
                candidateFocused_ = false;
                candidateCursorIndex_ = -1;
                isClauseBoundaryAdjusting_ = false;
                showNonPredictCandidateList(false);
                auto out = buildCurrentOutput(true);
                out.committed_text = text;
                return out;
            }
            return buildCommitOutput(text);
        }
    }

    if (event.isInputable()) {
        lastCommittedText_ += commitText_;
        auto out = buildCommitOutput(commitText_);
        reset();
        connector_.inputChar(event.inputString());
        showPreeditCandidateList();
        auto current = buildCurrentOutput(true);
        out.preedit_segments = current.preedit_segments;
        out.preedit_cursor = current.preedit_cursor;
        out.visible_candidates = current.visible_candidates;
        out.candidate_total_size = current.candidate_total_size;
        out.candidate_focused_index = current.candidate_focused_index;
        return out;
    }

    return buildNotConsumedOutput();
}

bool HazkeyEmacsState::ctrlShortcutHandler(const KeyEvent& event) {
    char c = 0;
    if (event.keycode >= 'a' && event.keycode <= 'z')
        c = event.keycode;
    else if (event.keycode >= 'A' && event.keycode <= 'Z')
        c = event.keycode + 32;
    else
        return false;

    switch (c) {
        case 'u':
            directCharacterConversion(ConversionMode::Hiragana);
            return true;
        case 'i':
            directCharacterConversion(ConversionMode::KatakanaFullwidth);
            return true;
        case 'o':
            directCharacterConversion(ConversionMode::KatakanaHalfwidth);
            return true;
        case 'p':
            directCharacterConversion(ConversionMode::RawFullwidth);
            return true;
        case 't':
            directCharacterConversion(ConversionMode::RawHalfwidth);
            return true;
        case 'h':
            connector_.deleteLeft();
            showPreeditCandidateList();
            return true;
        default:
            return false;
    }
}

void HazkeyEmacsState::functionKeyHandler(const KeyEvent& event) {
    switch (event.special) {
        case KeyEvent::F6:
            directCharacterConversion(ConversionMode::Hiragana);
            break;
        case KeyEvent::F7:
            directCharacterConversion(ConversionMode::KatakanaFullwidth);
            break;
        case KeyEvent::F8:
            directCharacterConversion(ConversionMode::KatakanaHalfwidth);
            break;
        case KeyEvent::F9:
            directCharacterConversion(ConversionMode::RawFullwidth);
            break;
        case KeyEvent::F10:
            directCharacterConversion(ConversionMode::RawHalfwidth);
            break;
        default:
            return;
    }
    isDirectConversionMode_ = true;
}

void HazkeyEmacsState::directCharacterConversion(ConversionMode mode) {
    hazkey::commands::GetComposingString::CharType type;
    switch (mode) {
        case ConversionMode::Hiragana:
            type = hazkey::commands::GetComposingString_CharType_HIRAGANA;
            break;
        case ConversionMode::KatakanaFullwidth:
            type = hazkey::commands::GetComposingString_CharType_KATAKANA_FULL;
            break;
        case ConversionMode::KatakanaHalfwidth:
            type = hazkey::commands::GetComposingString_CharType_KATAKANA_HALF;
            break;
        case ConversionMode::RawFullwidth:
            type = hazkey::commands::GetComposingString_CharType_ALPHABET_FULL;
            break;
        case ConversionMode::RawHalfwidth:
            type = hazkey::commands::GetComposingString_CharType_ALPHABET_HALF;
            break;
    }
    std::string converted = connector_.getComposingText(type, commitText_);
    commitText_ = converted;
    preeditSegments_.clear();
    preeditSegments_.push_back({converted, true});
    livePreeditIndex_ = -1;
    directConversionCharType_ = type;
    candidates_.clear();
    candidateFocused_ = false;
    candidateCursorIndex_ = -1;
    isDirectConversionMode_ = true;
}

bool HazkeyEmacsState::fetchCandidateList(bool isSuggest) {
    auto response = connector_.getCandidates(isSuggest);

    candidates_.clear();
    for (const auto& c : response.candidates()) {
        candidates_.push_back({c.text(), c.sub_hiragana()});
    }

    if (!response.live_text().empty()) {
        commitText_ = response.live_text();
        preeditSegments_.clear();
        int stableLen = response.stable_prefix_length();
        // UTF-8 character boundary split
        int bytePos = 0;
        int charCount = 0;
        const std::string& text = response.live_text();
        while (bytePos < (int)text.size() && charCount < stableLen) {
            unsigned char ch = text[bytePos];
            if (ch < 0x80)
                bytePos += 1;
            else if (ch < 0xE0)
                bytePos += 2;
            else if (ch < 0xF0)
                bytePos += 3;
            else
                bytePos += 4;
            charCount++;
        }
        std::string prefix = text.substr(0, bytePos);
        std::string trailing = text.substr(bytePos);
        if (!prefix.empty())
            preeditSegments_.push_back({prefix, false});
        if (!trailing.empty())
            preeditSegments_.push_back({trailing, true});
        else
            preeditSegments_.push_back({text, true});
        if (!response.trailing_clause_yomi().empty())
            preeditSegments_.push_back(
                {"[" + response.trailing_clause_yomi() + "]", false});
    } else {
        std::string hiragana = connector_.getComposingText(
            hazkey::commands::GetComposingString_CharType_HIRAGANA,
            commitText_);
        commitText_ = hiragana;
        preeditSegments_.clear();
        preeditSegments_.push_back({hiragana, false});
    }

    livePreeditIndex_ = response.live_text_index();

    if (response.page_size() > 0) {
        candidatePageSize_ =
            std::min(static_cast<int>(response.page_size()), 10);
        return true;
    }
    return false;
}

void HazkeyEmacsState::showNonPredictCandidateList(bool preserveTarget) {
    if (!preserveTarget) {
        connector_.moveCursor(1024);
        isClauseBoundaryAdjusting_ = false;
    }
    fetchCandidateList(false);
    livePreeditIndex_ = -1;

    // Highlight all preedit
    preeditSegments_.clear();
    preeditSegments_.push_back({commitText_, true});

    candidateFocused_ = true;
    candidateCursorIndex_ = 0;
    candidateIsConversion_ = true;
    updatePreeditFromCandidate();
}

bool HazkeyEmacsState::showPreeditCandidateList() {
    std::string composing = connector_.getComposingText(
        hazkey::commands::GetComposingString_CharType_HIRAGANA, commitText_);
    if (composing.empty()) {
        reset();
        return false;
    }
    candidateFocused_ = false;
    candidateCursorIndex_ = -1;
    candidateIsConversion_ = false;
    return fetchCandidateList(true);
}

void HazkeyEmacsState::advanceCandidateCursor() {
    if (candidateCursorIndex_ < (int)candidates_.size() - 1)
        candidateCursorIndex_++;
    updatePreeditFromCandidate();
}

void HazkeyEmacsState::backCandidateCursor() {
    if (candidateCursorIndex_ > 0) candidateCursorIndex_--;
    updatePreeditFromCandidate();
}

void HazkeyEmacsState::moveSegmentBoundary(bool expand) {
    isClauseBoundaryAdjusting_ = true;
    connector_.moveCursor(expand ? 1 : -1);
    showNonPredictCandidateList(true);
}

void HazkeyEmacsState::updatePreeditFromCandidate() {
    if (candidateCursorIndex_ >= 0 &&
        candidateCursorIndex_ < (int)candidates_.size()) {
        auto& cand = candidates_[candidateCursorIndex_];
        commitText_ = cand.text;
        preeditSegments_.clear();
        preeditSegments_.push_back({cand.text, true});
        if (!cand.hiragana.empty()) {
            preeditSegments_.push_back({cand.hiragana, false});
        }
    }
}

void HazkeyEmacsState::reset() {
    isDirectConversionMode_ = false;
    directConversionCharType_ = std::nullopt;
    livePreeditIndex_ = -1;
    isCursorMoving_ = false;
    isClauseBoundaryAdjusting_ = false;
    commitText_.clear();
    preeditSegments_.clear();
    candidates_.clear();
    candidateFocused_ = false;
    candidateCursorIndex_ = -1;
    candidateIsConversion_ = false;
    connector_.newComposingText();
}

OutputData HazkeyEmacsState::buildCurrentOutput(bool consumed) {
    OutputData out;
    out.consumed = consumed;
    out.preedit_segments = preeditSegments_;
    out.preedit_cursor = preeditCursor_;

    if (!candidates_.empty()) {
        int pageStart = 0;
        if (candidateCursorIndex_ >= 0) {
            pageStart = (candidateCursorIndex_ / candidatePageSize_) *
                        candidatePageSize_;
        }
        int pageEnd =
            std::min(pageStart + candidatePageSize_, (int)candidates_.size());
        for (int i = pageStart; i < pageEnd; i++) {
            int localIdx = i - pageStart;
            std::string shortcut =
                localIdx < (int)SELECTION_KEYS.size()
                    ? std::string(1, SELECTION_KEYS[localIdx])
                    : "";
            out.visible_candidates.push_back(
                {candidates_[i].text, i, shortcut});
        }
        out.candidate_focused_index =
            candidateFocused_ ? candidateCursorIndex_ : -1;
        out.candidate_total_size = candidates_.size();
        out.candidate_is_conversion = candidateIsConversion_;
    }

    return out;
}

OutputData HazkeyEmacsState::buildCommitOutput(const std::string& text) {
    OutputData out;
    out.consumed = true;
    out.committed_text = text;
    lastCommittedText_ += text;
    reset();
    return out;
}

OutputData HazkeyEmacsState::buildNotConsumedOutput() {
    OutputData out;
    out.consumed = false;
    return out;
}
