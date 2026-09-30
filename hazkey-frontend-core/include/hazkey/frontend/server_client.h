#ifndef HAZKEY_FRONTEND_SERVER_CLIENT_H_
#define HAZKEY_FRONTEND_SERVER_CLIENT_H_

#include <optional>
#include <string>

#include "commands.pb.h"
#include "hazkey/frontend/server_api.h"
#include "hazkey/frontend/server_connection.h"

namespace hazkey::frontend {

// ServerApi over a ServerConnection. Errors are logged through the hooks and
// reported as empty results.
class ServerClient : public ServerApi {
   public:
    ServerClient(ServerConnection& connection, ConnectionHooks& hooks)
        : connection_(connection), hooks_(hooks) {}

    std::string getComposingText(CharType type,
                                 const std::string& currentPreedit) override;
    TextWithCursor getComposingHiraganaWithCursor() override;
    void inputChar(const std::string& text) override;
    void shiftKeyEvent(bool isRelease) override;
    bool currentInputModeIsDirect() override;
    void deleteLeft() override;
    void deleteRight() override;
    void moveCursor(int offset) override;
    void setContext(const std::string& context, int anchor) override;
    void newComposingText() override;
    void completePrefix(int index) override;
    void directConversionComplete(CharType charType) override;
    void deleteTrailingClause() override;
    std::string completePrefixClauses() override;
    void insertHiragana(const std::string& text) override;
    hazkey::commands::CandidatesResult getCandidates(bool isSuggest) override;
    KeyBindings getKeyBindings() override;

    // tryConnect = false when the server must not be started, e.g. while
    // the session is shutting down.
    void saveLearningData(bool tryConnect = true);

   private:
    // Send the request and return the response if it succeeded. Logs the
    // error with the command name otherwise.
    std::optional<hazkey::ResponseEnvelope> request(
        const char* name, const hazkey::RequestEnvelope& request,
        bool tryConnect = true);

    ServerConnection& connection_;
    ConnectionHooks& hooks_;

    // the key bindings last received, serialized, and the result of reading
    // them. read again only when they change, so that errors are logged once.
    std::string lastKeyBindings_;
    KeyBindings keyBindings_;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_SERVER_CLIENT_H_
