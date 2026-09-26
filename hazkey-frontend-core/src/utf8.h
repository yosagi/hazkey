#ifndef HAZKEY_FRONTEND_UTF8_H_
#define HAZKEY_FRONTEND_UTF8_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace hazkey::frontend::utf8 {

// Byte length of the first `nchars` characters of `text`.
size_t byteLengthOfChars(const std::string& text, size_t nchars);
// Code point of the last character. 0 if `text` is empty.
uint32_t lastCodePoint(const std::string& text);

}  // namespace hazkey::frontend::utf8

#endif  // HAZKEY_FRONTEND_UTF8_H_
