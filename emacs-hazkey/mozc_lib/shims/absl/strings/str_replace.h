#pragma once
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace absl {

inline std::string StrReplaceAll(
    std::string_view s,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        replacements) {
    std::string result(s);
    for (const auto& [from, to] : replacements) {
        std::string::size_type pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    return result;
}

}  // namespace absl
