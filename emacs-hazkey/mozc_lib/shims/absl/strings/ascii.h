#pragma once
#include <cctype>

namespace absl {

inline bool ascii_isspace(char c) {
    return std::isspace(static_cast<unsigned char>(c));
}

inline bool ascii_isgraph(char c) {
    return std::isgraph(static_cast<unsigned char>(c));
}

}  // namespace absl
