#include "hazkey/frontend/state_machine.h"

#include <algorithm>

#include "hazkey/frontend/keysyms.h"
#include "utf8.h"

namespace hazkey::frontend {

namespace {

constexpr int kMaxPageSize = 10;

constexpr auto kHiragana = hazkey::commands::GetComposingString_CharType_HIRAGANA;

bool isKanji(uint32_t cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF);
}

}  // namespace

/// CandidateWindow

int CandidateWindow::pageCount() const {
    if (pageSize <= 0) return 0;
    return (static_cast<int>(items.size()) + pageSize - 1) / pageSize;
}

int CandidateWindow::pageItemCount() const {
    int remaining = static_cast<int>(items.size()) - pageStart();
    return std::clamp(remaining, 0, pageSize);
}

/// StateMachine

StateMachine::StateMachine(ServerApi& server, FrontendHooks& hooks)
    : server_(server), hooks_(hooks) {
    server_.newComposingText();
}

Output StateMachine::finish(KeyResult result) {
    Output out;
    out.result = result;
    out.commit = std::move(pendingCommit_);
    pendingCommit_.clear();
    out.preedit = preedit_;
    out.auxUp = auxUp_;
    out.auxDown = auxDown_;
    out.candidates = candidates_;
    return out;
}

Output StateMachine::keyEvent(const KeyEvent& event) {
    std::string composingText =
        server_.getComposingText(kHiragana, preeditText_);

    if (event.sym == keysym::Shift_L || event.sym == keysym::Shift_R) {
        server_.shiftKeyEvent(event.isRelease);
        if (composingText.empty()) {
            setAuxDown(AuxDown::None);
            return finish(KeyResult::Ignored);
        }
    }

    bool consumed = false;
    if (candidates_.visible && candidates_.focused() && !event.isRelease) {
        consumed = candidateKeyEvent(event);
    } else if (!composingText.empty() && !event.isRelease) {
        consumed = preeditKeyEvent(event);
    } else if (!event.isRelease) {
        consumed = noPreeditKeyEvent(event);
    } else if (!composingText.empty() && candidates_.visible &&
               !candidates_.focused() && hooks_.showTabToSelect()) {
        setAuxDown(AuxDown::TabToSelect);
    } else {
        setAuxDown(AuxDown::None);
    }

    if (event.isRelease) {
        return finish(KeyResult::Ignored);
    }

    if (candidates_.visible && candidates_.focused()) {
        setCandidateCursorAux();
    } else if (!composingText.empty()) {
        setHiraganaAux();
    }
    return finish(consumed ? KeyResult::Consumed
                           : KeyResult::PassToApplication);
}

Output StateMachine::reset() {
    resetState();
    return finish(KeyResult::Consumed);
}

Output StateMachine::commitAndReset() {
    commitPreedit();
    resetState();
    return finish(KeyResult::Consumed);
}

bool StateMachine::noPreeditKeyEvent(const KeyEvent& event) {
    if (event.sym == keysym::Space) {
        if (event.mods == mod::Shift) {
            commit(" ");
            resetState();
        } else {
            server_.inputChar(" ");
            commit(server_.getComposingText(kHiragana, ""));
            resetState();
        }
        return true;
    }

    if (!event.text.empty()) {
        updateSurroundingText();
        server_.inputChar(event.text);
        showPreeditCandidateList();
        setHiraganaAux();
        return true;
    }

    resetState();
    return false;
}

bool StateMachine::preeditKeyEvent(const KeyEvent& event) {
    auto sym = event.sym;

    if (sym == keysym::Return && event.mods == mod::Shift) {
        completePrefixAndCommit();
        return true;
    }
    if (sym == keysym::BackSpace && event.mods == mod::Shift) {
        shelveTrailingClause();
        return true;
    }
    switch (sym) {
        case keysym::Return:
            commitPreedit();
            if (livePreeditIndex_ >= 0) {
                server_.completePrefix(livePreeditIndex_);
            } else if (directConversionCharType_.has_value()) {
                server_.directConversionComplete(
                    directConversionCharType_.value());
            }
            resetState();
            restoreShelvedReadings();
            break;
        case keysym::BackSpace:
            server_.deleteLeft();
            showPreeditCandidateList();
            break;
        case keysym::Delete:
            server_.deleteRight();
            showPreeditCandidateList();
            break;
        case keysym::F6:
        case keysym::F7:
        case keysym::F8:
        case keysym::F9:
        case keysym::F10:
        case keysym::Muhenkan:
            functionKeyHandler(sym);
            break;
        case keysym::Escape:
            clearShelvedReadings();
            resetState();
            break;
        case keysym::Space:
            if (!isDirectConversionMode_ && event.mods == mod::Shift) {
                server_.inputChar(" ");
                showPreeditCandidateList();
            } else {
                showNonPredictCandidateList();
            }
            break;
        case keysym::Henkan:
            showNonPredictCandidateList();
            break;
        case keysym::Up:
        case keysym::Down:
        case keysym::Tab:
            if (!candidates_.visible) {
                showNonPredictCandidateList();
            } else {
                candidates_.cursor = 0;
                candidates_.page = 0;
                updateCandidateCursor();
            }
            break;
        case keysym::Left:
            isCursorMoving_ = true;
            server_.moveCursor(-1);
            break;
        case keysym::Right:
            if (isCursorMoving_) {
                server_.moveCursor(1);
            }
            break;
        default:
            if (event.mods == (mod::Ctrl | mod::Shift) &&
                (sym == keysym::h || sym == keysym::H)) {
                shelveTrailingClause();
            } else if (event.mods == mod::Ctrl) {
                ctrlShortcutHandler(sym);
            } else if (isAltDigitKeyEvent(event)) {
                if (candidates_.visible) {
                    int localIndex = static_cast<int>(sym - keysym::Digit1);
                    if (localIndex < candidates_.pageItemCount()) {
                        candidates_.cursor =
                            candidates_.pageStart() + localIndex;
                        candidateCompleteHandler();
                    }
                }
            } else if (!event.text.empty()) {
                clearShelvedReadings();
                if (isDirectConversionMode_) {
                    commitPreedit();
                    resetState();
                }
                server_.inputChar(event.text);
                showPreeditCandidateList();
            }
            break;
    }
    return true;
}

bool StateMachine::isAltDigitKeyEvent(const KeyEvent& event) {
    return event.mods == mod::Alt && event.sym >= keysym::Digit1 &&
           event.sym <= keysym::Digit9;
}

// index of the candidate selected by 1-9, 0 keys, or -1
int StateMachine::selectionKeyIndex(const KeyEvent& event) {
    if (event.mods != mod::None) return -1;
    if (event.sym >= keysym::Digit1 && event.sym <= keysym::Digit9) {
        return static_cast<int>(event.sym - keysym::Digit1);
    }
    if (event.sym == keysym::Digit0) return 9;
    return -1;
}

bool StateMachine::candidateKeyEvent(const KeyEvent& event) {
    auto sym = event.sym;

    switch (sym) {
        case keysym::Right:
            if (event.mods == mod::Shift) {
                moveSegmentBoundary(true);
            } else {
                nextCandidatePage();
            }
            break;
        case keysym::Left:
            if (event.mods == mod::Shift) {
                moveSegmentBoundary(false);
            } else {
                prevCandidatePage();
            }
            break;
        case keysym::Return:
            completedWithNoRemaining_ = false;
            candidateCompleteHandler();
            if (completedWithNoRemaining_) {
                restoreShelvedReadings();
            }
            break;
        case keysym::Escape:
            if (isClauseBoundaryAdjusting_) {
                showNonPredictCandidateList(false);
                break;
            }
            isClauseBoundaryAdjusting_ = false;
            [[fallthrough]];
        case keysym::BackSpace:
            showPreeditCandidateList();
            break;
        case keysym::Space:
        case keysym::Tab:
            if (event.mods == mod::Shift) {
                backCandidateCursor();
            } else if (event.mods == (mod::Alt | mod::Shift)) {
                // do nothing
            } else {
                advanceCandidateCursor();
            }
            break;
        case keysym::Down:
            advanceCandidateCursor();
            break;
        case keysym::Up:
            backCandidateCursor();
            break;
        case keysym::F6:
        case keysym::F7:
        case keysym::F8:
        case keysym::F9:
        case keysym::F10:
            functionKeyHandler(sym);
            break;
        default: {
            int selection = isAltDigitKeyEvent(event)
                                ? static_cast<int>(sym - keysym::Digit1)
                                : selectionKeyIndex(event);
            if (event.mods == mod::Ctrl) {
                if (!ctrlShortcutHandler(sym)) {
                    return false;
                }
            } else if (selection >= 0) {
                if (selection < candidates_.pageItemCount()) {
                    completedWithNoRemaining_ = false;
                    candidates_.cursor = candidates_.pageStart() + selection;
                    candidateCompleteHandler();
                    if (completedWithNoRemaining_) {
                        restoreShelvedReadings();
                    }
                }
            } else if (!event.text.empty()) {
                clearShelvedReadings();
                commitPreedit();
                resetState();
                server_.inputChar(event.text);
                showPreeditCandidateList();
            } else {
                return false;
            }
            break;
        }
    }
    return true;
}

void StateMachine::candidateCompleteHandler() {
    if (candidates_.cursor < 0 ||
        candidates_.cursor >= static_cast<int>(candidates_.items.size())) {
        return;
    }
    auto candidate = candidates_.items[candidates_.cursor];
    // hazkey cannot get surroundingText correctly immediately after
    // committing so call it with appendText before committing.
    updateSurroundingText(candidate.text);
    server_.completePrefix(candidates_.cursor);
    commit(candidate.text);
    if (!candidate.subHiragana.empty()) {
        isClauseBoundaryAdjusting_ = false;
        showNonPredictCandidateList(false);
    } else {
        resetState();
        completedWithNoRemaining_ = true;
    }
}

void StateMachine::updateSurroundingText(const std::string& appendText) {
    auto surrounding = hooks_.surroundingText();
    if (surrounding.has_value()) {
        server_.setContext(surrounding->text + appendText,
                           surrounding->anchor + static_cast<int>(appendText.length()));
    } else {
        server_.setContext("", 0);
    }
}

bool StateMachine::ctrlShortcutHandler(uint32_t sym) {
    switch (sym) {
        case keysym::u:
        case keysym::U:
            directCharacterConversion(ConversionMode::Hiragana);
            isDirectConversionMode_ = true;
            break;
        case keysym::i:
        case keysym::I:
            directCharacterConversion(ConversionMode::KatakanaFullwidth);
            isDirectConversionMode_ = true;
            break;
        case keysym::o:
        case keysym::O:
            directCharacterConversion(ConversionMode::KatakanaHalfwidth);
            isDirectConversionMode_ = true;
            break;
        case keysym::p:
        case keysym::P:
            directCharacterConversion(ConversionMode::RawFullwidth);
            isDirectConversionMode_ = true;
            break;
        case keysym::t:
        case keysym::T:
            directCharacterConversion(ConversionMode::RawHalfwidth);
            isDirectConversionMode_ = true;
            break;
        case keysym::h:
        case keysym::H:
            server_.deleteLeft();
            showPreeditCandidateList();
            break;
        default:
            return false;
    }
    return true;
}

void StateMachine::functionKeyHandler(uint32_t sym) {
    switch (sym) {
        case keysym::F6:
            directCharacterConversion(ConversionMode::Hiragana);
            break;
        case keysym::F7:
            directCharacterConversion(ConversionMode::KatakanaFullwidth);
            break;
        case keysym::F8:
            directCharacterConversion(ConversionMode::KatakanaHalfwidth);
            break;
        case keysym::F9:
            directCharacterConversion(ConversionMode::RawFullwidth);
            break;
        case keysym::F10:
            directCharacterConversion(ConversionMode::RawHalfwidth);
            break;
        default:
            return;
    }
    isDirectConversionMode_ = true;
}

void StateMachine::directCharacterConversion(ConversionMode mode) {
    CharType charType = kHiragana;
    switch (mode) {
        case ConversionMode::Hiragana:
            charType = kHiragana;
            break;
        case ConversionMode::KatakanaFullwidth:
            charType =
                hazkey::commands::GetComposingString_CharType_KATAKANA_FULL;
            break;
        case ConversionMode::KatakanaHalfwidth:
            charType =
                hazkey::commands::GetComposingString_CharType_KATAKANA_HALF;
            break;
        case ConversionMode::RawFullwidth:
            charType =
                hazkey::commands::GetComposingString_CharType_ALPHABET_FULL;
            break;
        case ConversionMode::RawHalfwidth:
            charType =
                hazkey::commands::GetComposingString_CharType_ALPHABET_HALF;
            break;
    }
    std::string converted = server_.getComposingText(charType, preeditText_);
    setSimplePreeditHighlighted(converted);
    livePreeditIndex_ = -1;
    directConversionCharType_ = charType;
    if (candidates_.visible) {
        candidates_ = CandidateWindow();
        setAuxDown(AuxDown::None);
    }
}

/// Candidate list

bool StateMachine::showCandidateList(bool isSuggest) {
    auto response = server_.getCandidates(isSuggest);

    resetPanel();

    if (!response.live_text().empty()) {
        lastTrailingClauseYomi_ = response.trailing_clause_yomi();
        auto furigana = lastTrailingClauseYomi_;
        // furigana is useful only when the trailing clause ends with kanji
        if (!furigana.empty() &&
            !isKanji(utf8::lastCodePoint(response.live_text()))) {
            furigana.clear();
        }
        setSimplePreeditWithFurigana(response.live_text(),
                                     response.stable_prefix_length(), furigana);
    } else {
        // preedit conversion is disabled or conversion result is not
        // available. show hiragana preedit
        setSimplePreedit(server_.getComposingText(kHiragana, preeditText_));
    }

    livePreeditIndex_ = response.live_text_index();

    if (response.page_size() > 0) {
        candidates_.visible = true;
        for (const auto& c : response.candidates()) {
            candidates_.items.push_back({c.text(), c.sub_hiragana()});
        }
        candidates_.pageSize =
            std::min(static_cast<int>(response.page_size()), kMaxPageSize);
    }

    // true if the list is displayed
    return response.page_size() > 0;
}

void StateMachine::showNonPredictCandidateList(bool preserveTarget) {
    if (!preserveTarget) {
        server_.moveCursor(1024);
        isClauseBoundaryAdjusting_ = false;
    }
    showCandidateList(false);

    livePreeditIndex_ = -1;

    // highlight all preedit text
    // because the first candidate is the result of all preedit text.
    setSimplePreeditHighlighted(preeditText_);

    if (!candidates_.visible) {
        return;
    }
    candidates_.isConversion = true;
    candidates_.cursor = 0;
    candidates_.page = 0;
    updateCandidateCursor();
}

void StateMachine::showPreeditCandidateList() {
    if (server_.getComposingText(kHiragana, preeditText_).empty()) {
        resetState();
        return;
    }
    if (showCandidateList(true) && hooks_.showTabToSelect()) {
        setAuxDown(AuxDown::TabToSelect);
    } else {
        setAuxDown(AuxDown::None);
    }
}

/// Candidate cursor

void StateMachine::updateCandidateCursor() {
    setCandidateCursorAux();
    const auto& candidate = candidates_.items[candidates_.cursor];
    if (candidate.subHiragana.empty()) {
        setMultiSegmentPreedit({candidate.text}, 0);
    } else {
        setMultiSegmentPreedit({candidate.text, candidate.subHiragana}, 0);
    }
}

// move to the next candidate. wraps around at the end.
void StateMachine::advanceCandidateCursor() {
    int size = static_cast<int>(candidates_.items.size());
    if (size == 0) return;
    candidates_.cursor = (candidates_.cursor + 1) % size;
    candidates_.page = candidates_.cursor / candidates_.pageSize;
    updateCandidateCursor();
}

// move to the previous candidate. wraps around at the beginning.
void StateMachine::backCandidateCursor() {
    int size = static_cast<int>(candidates_.items.size());
    if (size == 0) return;
    candidates_.cursor = (candidates_.cursor - 1 + size) % size;
    candidates_.page = candidates_.cursor / candidates_.pageSize;
    updateCandidateCursor();
}

// move to the top of the next page. stays on the last page.
void StateMachine::nextCandidatePage() {
    if (candidates_.page + 1 < candidates_.pageCount()) {
        candidates_.page++;
    }
    candidates_.cursor = candidates_.pageStart();
    updateCandidateCursor();
}

// move to the top of the previous page. stays on the first page.
void StateMachine::prevCandidatePage() {
    if (candidates_.page > 0) {
        candidates_.page--;
    }
    candidates_.cursor = candidates_.pageStart();
    updateCandidateCursor();
}

void StateMachine::moveSegmentBoundary(bool expand) {
    isClauseBoundaryAdjusting_ = true;
    server_.moveCursor(expand ? 1 : -1);
    showNonPredictCandidateList(true);
}

/// Aux

void StateMachine::setCandidateCursorAux() {
    auto label = "[" + std::to_string(candidates_.cursor + 1) + "/" +
                 std::to_string(candidates_.items.size()) + "]";
    auxUp_ = {{label, SegmentStyle::Normal}};
    setAuxDown(AuxDown::None);
}

void StateMachine::setAuxDown(AuxDown aux) {
    if (server_.currentInputModeIsDirect()) {
        auxDown_ = AuxDown::DirectInput;
    } else {
        auxDown_ = aux;
    }
}

void StateMachine::setHiraganaAux() {
    auto text = server_.getComposingHiraganaWithCursor();
    auxUp_ = {{text.before, SegmentStyle::Normal},
              {text.on, SegmentStyle::Underline},
              {text.after, SegmentStyle::Normal}};
}

/// Clause partial operations

void StateMachine::shelveTrailingClause() {
    if (lastTrailingClauseYomi_.empty()) {
        return;
    }
    shelvedReadings_.push_back(lastTrailingClauseYomi_);
    server_.deleteTrailingClause();
    lastTrailingClauseYomi_.clear();
    showPreeditCandidateList();
}

void StateMachine::completePrefixAndCommit() {
    auto prefixText = server_.completePrefixClauses();
    if (!prefixText.empty()) {
        commit(prefixText);
    }
    showPreeditCandidateList();
}

void StateMachine::restoreShelvedReadings() {
    if (shelvedReadings_.empty()) {
        return;
    }
    std::string combined;
    for (auto it = shelvedReadings_.rbegin(); it != shelvedReadings_.rend();
         ++it) {
        combined += *it;
    }
    shelvedReadings_.clear();
    server_.insertHiragana(combined);
    showPreeditCandidateList();
}

void StateMachine::clearShelvedReadings() { shelvedReadings_.clear(); }

/// Preedit

// segments before cursorSegment are plain, the one at cursorSegment is
// highlighted with the caret at its start, and the rest are underlined.
void StateMachine::setMultiSegmentPreedit(const std::vector<std::string>& texts,
                                          int cursorSegment) {
    preeditText_.clear();
    preedit_ = Preedit();
    for (int i = 0; static_cast<size_t>(i) < texts.size(); i++) {
        preeditText_ += texts[i];
        SegmentStyle style;
        if (i < cursorSegment) {
            style = SegmentStyle::Normal;
        } else if (i == cursorSegment) {
            preedit_.caretSegment = i;
            style = SegmentStyle::Highlight;
        } else {
            style = SegmentStyle::Underline;
        }
        preedit_.segments.push_back({texts[i], style});
    }
}

void StateMachine::setSimplePreeditHighlighted(const std::string& text) {
    setMultiSegmentPreedit({text}, 0);
}

void StateMachine::setSimplePreedit(const std::string& text) {
    setMultiSegmentPreedit({text}, -1);
}

void StateMachine::setSimplePreeditWithFurigana(const std::string& text,
                                                int stablePrefixLen,
                                                const std::string& furigana) {
    preeditText_ = text;
    preedit_ = Preedit();
    auto stablePrefixBytes = utf8::byteLengthOfChars(text, stablePrefixLen);
    auto stablePrefix = text.substr(0, stablePrefixBytes);
    auto trailingClause = text.substr(stablePrefixBytes);
    if (!stablePrefix.empty()) {
        preedit_.segments.push_back({stablePrefix, SegmentStyle::Underline});
    }
    preedit_.segments.push_back({trailingClause, SegmentStyle::Highlight});
    if (!furigana.empty()) {
        preedit_.segments.push_back(
            {"[" + furigana + "]", SegmentStyle::Normal});
    }
}

void StateMachine::commitPreedit() {
    commit(preeditText_);
    preeditText_.clear();
}

void StateMachine::commit(const std::string& text) { pendingCommit_ += text; }

/// Reset

void StateMachine::resetPanel() {
    preedit_ = Preedit();
    auxUp_.clear();
    auxDown_ = AuxDown::None;
    candidates_ = CandidateWindow();
}

void StateMachine::resetState() {
    isDirectConversionMode_ = false;
    directConversionCharType_ = std::nullopt;
    livePreeditIndex_ = -1;
    isCursorMoving_ = false;
    isClauseBoundaryAdjusting_ = false;
    server_.newComposingText();
    preeditText_.clear();
    resetPanel();
}

}  // namespace hazkey::frontend
