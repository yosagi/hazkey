#include "hazkey_preedit.h"

namespace fcitx {

namespace {

TextFormatFlag toFormatFlag(hazkey::frontend::SegmentStyle style) {
    switch (style) {
        case hazkey::frontend::SegmentStyle::Underline:
            return TextFormatFlag::Underline;
        case hazkey::frontend::SegmentStyle::Highlight:
            return TextFormatFlag::HighLight;
        case hazkey::frontend::SegmentStyle::Normal:
        default:
            return TextFormatFlag::NoFlag;
    }
}

}  // namespace

Text toFcitxText(const std::vector<hazkey::frontend::Segment> &segments,
                 int caretSegment) {
    auto text = Text();
    for (int i = 0; size_t(i) < segments.size(); i++) {
        if (i == caretSegment) {
            text.setCursor(text.textLength());
        }
        text.append(segments[i].text, toFormatFlag(segments[i].style));
    }
    return text;
}

void HazkeyPreedit::render(const hazkey::frontend::Preedit &preedit) {
    setPreedit(toFcitxText(preedit.segments, preedit.caretSegment));
}

void HazkeyPreedit::setPreedit(Text text) {
    if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
        ic_->inputPanel().setClientPreedit(text);
    } else {
        ic_->inputPanel().setPreedit(text);
    }
}

}  // namespace fcitx
