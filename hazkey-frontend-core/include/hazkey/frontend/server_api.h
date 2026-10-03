#ifndef HAZKEY_FRONTEND_SERVER_API_H_
#define HAZKEY_FRONTEND_SERVER_API_H_

#include <string>

#include "commands.pb.h"
#include "hazkey/frontend/keybindings.h"

namespace hazkey::frontend {

// Settings the clients read from the profile through the server.
struct ClientSettings {
    // key bindings from the config, merged with the defaults
    KeyBindings keyBindings;
    // readings kept for idle.restore_reading. 0 disables it
    int readingHistorySize = 5;
};

struct TextWithCursor {
    std::string before;
    std::string on;
    std::string after;
    // position of the cursor on the reading, in characters
    int readingCursor = 0;
    // position on the text of the auto-conversion that corresponds to the
    // cursor, in characters. -1 when the cursor is not on a boundary
    // between elements or there is no auto-conversion
    int surfaceCursor = -1;
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
    // move to the nearest boundary between elements of the auto-conversion:
    // before the cursor (direction < 0), after it (> 0), or at or before it
    // (0). By one character when the elements are not available
    virtual void moveCursorByElement(int direction) = 0;
    virtual void setContext(const std::string& context, int anchor) = 0;
    virtual void newComposingText() = 0;
    virtual void completePrefix(int index) = 0;
    virtual void directConversionComplete(CharType charType) = 0;
    // returns the deleted reading, or an empty string when nothing is deleted
    virtual std::string deleteTrailingClause() = 0;
    virtual std::string completePrefixClauses() = 0;
    virtual void insertHiragana(const std::string& text) = 0;
    virtual hazkey::commands::CandidatesResult getCandidates(
        bool isSuggest) = 0;
    virtual ClientSettings getClientSettings() = 0;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_SERVER_API_H_
