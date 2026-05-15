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
//   - Removed ParseInputLine, PrintMessage, PrintField, PrintFieldValue,
//     RemoveUsageData and their associated includes.
// Original: https://github.com/google/mozc/blob/master/src/unix/emacs/mozc_emacs_helper_lib.cc

#include "mozc_emacs_helper_lib.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "base/util.h"

namespace mozc {
namespace emacs {

// Normalizes a symbol with the following rules:
// - all alphabets are converted to lowercase
// - underscore('_') is converted to dash('-')
std::string NormalizeSymbol(absl::string_view symbol) {
  std::string normalized(symbol);
  Util::LowerString(&normalized);
  std::replace(normalized.begin(), normalized.end(), '_', '-');
  return normalized;
}

// Returns a quoted string as a string literal in S-expression.
// - double-quote is converted to backslash + double-quote
// - backslash is converted to backslash + backslash
//
// Control characters, including newline('\n'), in a given string remain as is.
std::string QuoteString(absl::string_view str) {
  return absl::StrCat(
      "\"", absl::StrReplaceAll(str, {{"\\", "\\\\"}, {"\"", "\\\""}}), "\"");
}

// Unquotes and unescapes a double-quoted string.
// The input string must begin and end with double quotes.
bool UnquoteString(absl::string_view input, std::string* output) {
  DCHECK(output);
  output->clear();

  if (input.length() < 2 || input.front() != '\"' || input.back() != '\"') {
    return false;  // wrong format
  }

  std::string result;
  result.reserve(input.size());

  bool escape = false;
  for (char c : input.substr(1, input.length() - 2)) {
    if (escape) {
      switch (c) {
        case 'a':
          c = '\x07';
          break;  // control-g
        case 'b':
          c = '\x08';
          break;  // backspace
        case 't':
          c = '\x09';
          break;  // tab
        case 'n':
          c = '\x0a';
          break;  // newline
        case 'v':
          c = '\x0b';
          break;  // vertical tab
        case 'f':
          c = '\x0c';
          break;  // formfeed
        case 'r':
          c = '\x0d';
          break;  // carriage return
        case 'e':
          c = '\x1b';
          break;  // escape
        case 's':
          c = '\x20';
          break;  // space
        case 'd':
          c = '\x7f';
          break;  // delete
      }
      result.push_back(c);
      escape = false;
    } else if (c == '\\') {
      escape = true;
    } else if (c == '\"') {
      return false;
    } else {
      result.push_back(c);
    }
  }

  if (escape) {  // wrong format
    return false;
  }
  *output = std::move(result);
  return true;
}

// Tokenizes the given string as S expression.  Returns true if success.
bool TokenizeSExpr(absl::string_view input, std::vector<std::string>* output) {
  DCHECK(output);

  std::vector<std::string> results;

  for (auto it = input.begin(); it != input.end(); ++it) {
    if (absl::ascii_isspace(*it)) {
      continue;
    }  // Skip white space.

    if (!absl::ascii_isgraph(*it)) {
      return false;  // unrecognized control character
    }

    switch (*it) {
      case ';':  // comment
        while (it != input.end() && *it != '\n') {
          ++it;
        }
        break;
      case '(':
      case ')':  // list parentheses
      case '[':
      case ']':   // vector parentheses
      case '\'':  // quote
      case '`':   // quasiquote
        results.emplace_back(1, *it);
        break;
      case '\"': {  // string
        const auto start = it++;
        for (bool escape = false;; ++it) {
          if (it == input.end()) {
            return false;  // unexpected end of string
          }
          if (escape) {
            escape = false;
          } else if (*it == '\\') {
            escape = true;
          } else if (*it == '\"') {
            break;
          }
        }
        results.emplace_back(start, it + 1);
        break;
      }
      default: {  // must be atom
        const auto start = it++;
        for (;; ++it) {
          if (it == input.end()) {
            break;
          }
          if (!absl::ascii_isgraph(*it)) {
            break;
          }
          bool is_special_char = false;
          switch (*it) {
            case ';':  // comment
            case '(':
            case ')':  // list parentheses
            case '[':
            case ']':   // vector parentheses
            case '\'':  // quote
            case '`':   // quasiquote
            case '\"':  // string
              is_special_char = true;
          }
          if (is_special_char) {
            break;
          }
        }
        results.emplace_back(start, it);
        --it;  // Put the last char back.
        break;
      }
    }
  }

  *output = std::move(results);
  return true;
}

// Prints an error message in S-expression and terminates with status code 1.
void ErrorExit(absl::string_view error, absl::string_view message) {
  absl::FPrintF(stdout, "((error . %s)(message . %s))\n", error,
                QuoteString(message));
  exit(1);
}

}  // namespace emacs
}  // namespace mozc
