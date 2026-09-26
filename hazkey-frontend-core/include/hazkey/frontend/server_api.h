#ifndef HAZKEY_FRONTEND_SERVER_API_H_
#define HAZKEY_FRONTEND_SERVER_API_H_

#include <string>

#include "commands.pb.h"

namespace hazkey::frontend {

struct TextWithCursor {
    std::string before;
    std::string on;
    std::string after;
};

// Operations on hazkey-server used by the state machine. Each frontend
// implements this with its own connection handling.
class ServerApi {
   public:
    using CharType = hazkey::commands::GetComposingString::CharType;

    virtual ~ServerApi() = default;

    virtual std::string getComposingText(CharType type,
                                         const std::string& currentPreedit) = 0;
    virtual TextWithCursor getComposingHiraganaWithCursor() = 0;
    virtual void inputChar(const std::string& text) = 0;
    virtual void shiftKeyEvent(bool isRelease) = 0;
    virtual bool currentInputModeIsDirect() = 0;
    virtual void deleteLeft() = 0;
    virtual void deleteRight() = 0;
    virtual void moveCursor(int offset) = 0;
    virtual void setContext(const std::string& context, int anchor) = 0;
    virtual void newComposingText() = 0;
    virtual void completePrefix(int index) = 0;
    virtual void directConversionComplete(CharType charType) = 0;
    virtual void deleteTrailingClause() = 0;
    virtual std::string completePrefixClauses() = 0;
    virtual void insertHiragana(const std::string& text) = 0;
    virtual hazkey::commands::CandidatesResult getCandidates(
        bool isSuggest) = 0;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_SERVER_API_H_
