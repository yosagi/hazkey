#pragma once
#include <cstdio>
#include <string>
#include <string_view>

namespace absl {

// Minimal FPrintF supporting %s with string_view arguments.
// Only the format patterns used by mozc_emacs_helper_lib are supported.
inline void FPrintF(FILE* f, const char* fmt, std::string_view a,
                    std::string_view b) {
    // "((error . %s)(message . %s))\n"
    const char* p = fmt;
    while (*p) {
        if (p[0] == '%' && p[1] == 's') {
            if (a.data()) {
                fwrite(a.data(), 1, a.size(), f);
                a = {};
            } else {
                fwrite(b.data(), 1, b.size(), f);
            }
            p += 2;
        } else {
            fputc(*p, f);
            ++p;
        }
    }
}

}  // namespace absl
