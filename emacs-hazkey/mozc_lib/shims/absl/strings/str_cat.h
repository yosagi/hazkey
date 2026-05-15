#pragma once
#include <string>
#include <string_view>

namespace absl {

inline std::string StrCat(std::string_view a, std::string_view b,
                          std::string_view c) {
    std::string result;
    result.reserve(a.size() + b.size() + c.size());
    result.append(a);
    result.append(b);
    result.append(c);
    return result;
}

}  // namespace absl
