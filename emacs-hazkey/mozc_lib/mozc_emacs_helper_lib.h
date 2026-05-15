// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Modified for hazkey_emacs_helper:
//   - Removed ParseInputLine (depends on mozc KeyParser)
//   - Removed PrintMessage (requires full protobuf, not LITE_RUNTIME)
//   - Removed RemoveUsageData (depends on mozc protocol)
// Original: https://github.com/google/mozc/blob/master/src/unix/emacs/mozc_emacs_helper_lib.h

#ifndef MOZC_EMACS_HELPER_LIB_H_
#define MOZC_EMACS_HELPER_LIB_H_

#include <string>
#include <vector>

#include "absl/strings/string_view.h"

namespace mozc {
namespace emacs {

// Error symbols used to call ErrorExit()
inline constexpr absl::string_view kErrFileError = "file-error";
inline constexpr absl::string_view kErrScanError = "scan-error";
inline constexpr absl::string_view kErrWrongNumberOfArguments =
    "wrong-number-of-arguments";
inline constexpr absl::string_view kErrWrongTypeArgument =
    "wrong-type-argument";
inline constexpr absl::string_view kErrVoidFunction = "void-function";
inline constexpr absl::string_view kErrSessionError = "session-error";

// Normalizes a symbol with the following rule:
// - all alphabets are converted to lowercase
// - underscore('_') is converted to dash('-')
std::string NormalizeSymbol(absl::string_view symbol);

// Returns a quoted string as a string literal in S-expression.
std::string QuoteString(absl::string_view str);

// Unquotes and unescapes a double-quoted string.
// The input string must begin and end with double quotes.
bool UnquoteString(absl::string_view input, std::string* output);

// Tokenizes the given string as S expression.  Returns true if success.
bool TokenizeSExpr(absl::string_view input, std::vector<std::string>* output);

// Prints an error message in S-expression and terminates with status code 1.
void ErrorExit(absl::string_view error, absl::string_view message);

}  // namespace emacs
}  // namespace mozc

#endif  // MOZC_EMACS_HELPER_LIB_H_
