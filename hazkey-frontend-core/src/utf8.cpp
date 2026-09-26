#include "utf8.h"

namespace hazkey::frontend::utf8 {

namespace {

size_t sequenceLength(unsigned char lead) {
    if (lead < 0x80) return 1;
    if (lead < 0xE0) return 2;
    if (lead < 0xF0) return 3;
    return 4;
}

}  // namespace

size_t byteLengthOfChars(const std::string& text, size_t nchars) {
    size_t pos = 0;
    for (size_t n = 0; n < nchars && pos < text.size(); n++) {
        pos += sequenceLength(static_cast<unsigned char>(text[pos]));
    }
    return pos < text.size() ? pos : text.size();
}

uint32_t lastCodePoint(const std::string& text) {
    if (text.empty()) return 0;
    size_t start = text.size() - 1;
    while (start > 0 &&
           (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
        start--;
    }
    auto lead = static_cast<unsigned char>(text[start]);
    size_t len = sequenceLength(lead);
    uint32_t cp;
    if (len == 1) {
        cp = lead;
    } else if (len == 2) {
        cp = lead & 0x1F;
    } else if (len == 3) {
        cp = lead & 0x0F;
    } else {
        cp = lead & 0x07;
    }
    for (size_t i = 1; i < len && start + i < text.size(); i++) {
        cp = (cp << 6) | (static_cast<unsigned char>(text[start + i]) & 0x3F);
    }
    return cp;
}

}  // namespace hazkey::frontend::utf8
