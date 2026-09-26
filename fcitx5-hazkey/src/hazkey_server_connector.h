#ifndef HAZKEY_SERVER_CONNECTOR_H
#define HAZKEY_SERVER_CONNECTOR_H

#include <fcitx-utils/log.h>
#include <fcitx/text.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

#include "base.pb.h"
#include "commands.pb.h"
#include "hazkey/frontend/server_api.h"

class HazkeyServerConnector : public hazkey::frontend::ServerApi {
   public:
    // HazkeyServerConnector();
    // ~HazkeyServerConnector();

    HazkeyServerConnector() {
        // kill_existing_hazkey_server();
        connectServer();
        FCITX_DEBUG() << "Connector initialized";
    };

    std::string getSocketPath();

    void connectServer();

    void startHazkeyServer(bool force_restart);

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

    void setServerConfig(int zenzaiEnabled, int zenzaiInferLimit,
                         int numberFullwidth, int symbolFullwidth,
                         int periodStyleIndex, int commaStyleIndex,
                         int spaceFullwidth, int tenCombining,
                         std::string profileText);

    void newComposingText() override;

    void completePrefix(int index) override;

    void directConversionComplete(
        hazkey::commands::GetComposingString::CharType charType) override;

    void deleteTrailingClause() override;

    std::string completePrefixClauses() override;

    void insertHiragana(const std::string& text) override;

    void saveLearningData(bool tryConnect = true);

    struct CandidateData {
        std::string candidateText;
        std::string subHiragana;
    };

    hazkey::commands::CandidatesResult getCandidates(bool isSuggest) override;

   private:
    bool retryConnect();
    bool isHazkeyServerRunning();
    bool requestSuccess(hazkey::ResponseEnvelope);
    int sock_ = -1;
    std::string socket_path_;
};

#endif  // HAZKEY_SERVER_CONNECTOR_H
