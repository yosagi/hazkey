#include "hazkey_preedit.h"

namespace fcitx {

std::string HazkeyPreedit::text() const { return commitText_; }

void HazkeyPreedit::setPreedit(Text text) {
    if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
        ic_->inputPanel().setClientPreedit(text);
    } else {
        ic_->inputPanel().setPreedit(text);
    }
}

void HazkeyPreedit::setSimplePreeditHighlighted(const std::string &text) {
    std::vector<std::string> texts = {text};
    setMultiSegmentPreedit(texts, 0);
}

void HazkeyPreedit::setSimplePreedit(const std::string &text) {
    std::vector<std::string> texts = {text};
    setMultiSegmentPreedit(texts, -1);
}

void HazkeyPreedit::setSimplePreeditWithFurigana(const std::string &text,
                                                  int /*stablePrefixLen*/,
                                                  const std::string &furigana) {
    commitText_ = text;
    auto preedit = Text();
    preedit.append(text, TextFormatFlag::Underline);
    preedit.append("[" + furigana + "]", TextFormatFlag::NoFlag);
    setPreedit(preedit);
}

void HazkeyPreedit::setMultiSegmentPreedit(std::vector<std::string> &texts,
                                           int cursorSegment = 0) {
    commitText_.clear();
    for (const auto &t : texts) {
        commitText_ += t;
    }
    auto preedit = Text();
    for (int i = 0; size_t(i) < texts.size(); i++) {
        if (i < cursorSegment) {
            preedit.append(texts[i], TextFormatFlag::NoFlag);
        } else if (i == cursorSegment) {
            preedit.setCursor(preedit.textLength());
            preedit.append(texts[i], TextFormatFlag::HighLight);
            continue;
        } else {
            preedit.append(texts[i], TextFormatFlag::Underline);
        }
    }
    setPreedit(preedit);
}

void HazkeyPreedit::commitPreedit() {
    ic_->commitString(commitText_);
    commitText_.clear();
}

void HazkeyPreedit::clear() { commitText_.clear(); }

}  // namespace fcitx
