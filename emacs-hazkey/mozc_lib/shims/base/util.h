#pragma once
#include <algorithm>
#include <cctype>
#include <string>

namespace mozc {

class Util {
   public:
    static void LowerString(std::string* str) {
        std::transform(str->begin(), str->end(), str->begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }
};

}  // namespace mozc
