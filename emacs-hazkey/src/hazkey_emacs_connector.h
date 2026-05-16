#ifndef HAZKEY_EMACS_CONNECTOR_H_
#define HAZKEY_EMACS_CONNECTOR_H_

#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>

#include "base.pb.h"
#include "commands.pb.h"

struct TextWithCursor {
    std::string before;
    std::string on;
    std::string after;
};

class HazkeyEmacsConnector {
   public:
    HazkeyEmacsConnector();
    ~HazkeyEmacsConnector();

    std::string getSocketPath();
    void connectServer();

    std::optional<hazkey::ResponseEnvelope> transact(
        const hazkey::RequestEnvelope& send_data, bool tryConnect = true);

    std::string getComposingText(
        hazkey::commands::GetComposingString::CharType type,
        const std::string& currentPreedit);

    TextWithCursor getComposingHiraganaWithCursor();

    void inputChar(const std::string& text);
    void shiftKeyEvent(bool isRelease);
    bool currentInputModeIsDirect();
    void deleteLeft();
    void deleteRight();
    void moveCursor(int offset);
    void setContext(const std::string& context, int anchor);
    void newComposingText();
    void completePrefix(int index);
    void saveLearningData(bool tryConnect = true);
    hazkey::commands::CandidatesResult getCandidates(bool isSuggest);

   private:
    void ensureDedicatedServer();
    bool startDedicatedServer();
    bool isSocketAlive(const std::string& socketPath);
    void cleanStaleLock();
    std::string getLockPath();
    std::string runtimeDir();

    int sock_ = -1;
    pid_t serverPid_ = -1;
};

#endif
