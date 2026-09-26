#ifndef HAZKEY_EMACS_CONNECTOR_H_
#define HAZKEY_EMACS_CONNECTOR_H_

#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>

#include "base.pb.h"
#include "commands.pb.h"
#include "hazkey/frontend/server_api.h"

class HazkeyEmacsConnector : public hazkey::frontend::ServerApi {
   public:
    HazkeyEmacsConnector();
    ~HazkeyEmacsConnector();

    std::string getSocketPath();
    void connectServer();

    std::optional<hazkey::ResponseEnvelope> transact(
        const hazkey::RequestEnvelope& send_data, bool tryConnect = true);

    std::string getComposingText(
        hazkey::commands::GetComposingString::CharType type,
        const std::string& currentPreedit) override;

    hazkey::frontend::TextWithCursor getComposingHiraganaWithCursor() override;

    void inputChar(const std::string& text) override;
    void shiftKeyEvent(bool isRelease) override;
    bool currentInputModeIsDirect() override;
    void deleteLeft() override;
    void deleteRight() override;
    void moveCursor(int offset) override;
    void setContext(const std::string& context, int anchor) override;
    void newComposingText() override;
    void completePrefix(int index) override;
    void directConversionComplete(
        hazkey::commands::GetComposingString::CharType charType) override;
    void deleteTrailingClause() override;
    std::string completePrefixClauses() override;
    void insertHiragana(const std::string& text) override;
    void saveLearningData(bool tryConnect = true);
    hazkey::commands::CandidatesResult getCandidates(bool isSuggest) override;

   private:
    void startHazkeyServer();

    int sock_ = -1;
};

#endif
