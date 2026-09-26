#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hazkey/frontend/frontend_hooks.h"
#include "hazkey/frontend/state_machine.h"
#include "hazkey_emacs_connector.h"
#include "key_translator.h"
#include "mozc_emacs_helper_lib.h"
#include "mozc_output_builder.h"

using mozc::emacs::ErrorExit;
using mozc::emacs::TokenizeSExpr;
using mozc::emacs::UnquoteString;
using mozc::emacs::kErrScanError;
using mozc::emacs::kErrWrongNumberOfArguments;
using mozc::emacs::kErrWrongTypeArgument;
using mozc::emacs::kErrVoidFunction;

enum CommandType { CREATE_SESSION, DELETE_SESSION, SEND_KEY };

// mozc.el does not tell the text around the cursor, so the text committed in
// this session is used as the left context instead.
class EmacsHooks : public hazkey::frontend::FrontendHooks {
   public:
    std::optional<hazkey::frontend::SurroundingText> surroundingText()
        override {
        return hazkey::frontend::SurroundingText{
            committedText_, static_cast<int>(committedText_.length())};
    }
    bool showTabToSelect() override { return false; }

    void appendCommitted(const std::string& text) { committedText_ += text; }

   private:
    std::string committedText_;
};

struct Session {
    explicit Session(HazkeyEmacsConnector& connector)
        : core(connector, hooks) {}

    EmacsHooks hooks;
    hazkey::frontend::StateMachine core;
};

struct ParsedCommand {
    uint32_t event_id = 0;
    uint32_t session_id = 0;
    CommandType command;
    std::vector<std::string> key_tokens;
};

static ParsedCommand parseInputLine(const std::string& line) {
    std::vector<std::string> tokens;
    if (!TokenizeSExpr(line, &tokens) || tokens.size() < 4 ||
        tokens.front() != "(" || tokens.back() != ")") {
        ErrorExit(kErrScanError, "S expression in the wrong format");
    }

    ParsedCommand cmd;

    // Event ID
    char* end = nullptr;
    cmd.event_id = std::strtoul(tokens[1].c_str(), &end, 10);
    if (*end != '\0') {
        ErrorExit(kErrWrongTypeArgument, "Event ID is not an integer");
    }

    // Command
    const std::string& func = tokens[2];
    if (func == "CreateSession") {
        cmd.command = CREATE_SESSION;
        if (tokens.size() != 4) {
            ErrorExit(kErrWrongNumberOfArguments,
                      "Wrong number of arguments");
        }
    } else if (func == "DeleteSession") {
        cmd.command = DELETE_SESSION;
        if (tokens.size() != 5) {
            ErrorExit(kErrWrongNumberOfArguments,
                      "Wrong number of arguments");
        }
        cmd.session_id = std::strtoul(tokens[3].c_str(), &end, 10);
        if (*end != '\0') {
            ErrorExit(kErrWrongTypeArgument,
                      "Session ID is not an integer");
        }
    } else if (func == "SendKey") {
        cmd.command = SEND_KEY;
        if (tokens.size() < 6) {
            ErrorExit(kErrWrongNumberOfArguments,
                      "Wrong number of arguments");
        }
        cmd.session_id = std::strtoul(tokens[3].c_str(), &end, 10);
        if (*end != '\0') {
            ErrorExit(kErrWrongTypeArgument,
                      "Session ID is not an integer");
        }
        // Key tokens: everything between session_id and closing paren
        for (size_t i = 4; i < tokens.size() - 1; i++) {
            cmd.key_tokens.push_back(tokens[i]);
        }
    } else {
        ErrorExit(kErrVoidFunction, "Unknown function");
    }

    return cmd;
}

static void redirectStderr() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    uid_t uid = getuid();
    std::string dir;
    if (xdg && xdg[0] != '\0') {
        dir = std::string(xdg) + "/hazkey-emacs";
    } else {
        dir = "/tmp/hazkey-runtime-" + std::to_string(uid) + "/hazkey-emacs";
    }
    mkdir(dir.c_str(), 0700);
    std::string logPath = dir + "/hazkey_emacs_helper.log";
    freopen(logPath.c_str(), "a", stderr);
}

int main() {
    redirectStderr();

    // Print greeting
    std::cout << MozcOutputBuilder::buildGreeting() << "\n";
    std::cout.flush();

    HazkeyEmacsConnector connector;
    uint32_t nextSessionId = 1;
    std::unordered_map<uint32_t, std::unique_ptr<Session>> sessions;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        auto cmd = parseInputLine(line);
        std::string response;

        switch (cmd.command) {
            case CREATE_SESSION: {
                uint32_t sid = nextSessionId++;
                sessions[sid] = std::make_unique<Session>(connector);
                response = MozcOutputBuilder::buildCreateSessionResponse(
                    cmd.event_id, sid);
                break;
            }
            case DELETE_SESSION: {
                auto it = sessions.find(cmd.session_id);
                if (it != sessions.end()) {
                    sessions.erase(it);
                }
                connector.saveLearningData();
                response = MozcOutputBuilder::buildDeleteSessionResponse(
                    cmd.event_id, cmd.session_id);
                break;
            }
            case SEND_KEY: {
                auto it = sessions.find(cmd.session_id);
                if (it == sessions.end()) {
                    // Auto-create session if not found
                    sessions[cmd.session_id] =
                        std::make_unique<Session>(connector);
                    it = sessions.find(cmd.session_id);
                }
                auto keyEvent = KeyTranslator::translate(cmd.key_tokens);
                auto output = it->second->core.keyEvent(keyEvent);
                it->second->hooks.appendCommitted(output.commit);
                response = MozcOutputBuilder::buildResponse(
                    cmd.event_id, cmd.session_id, output);
                break;
            }
        }

        std::cout << response << "\n";
        std::cout.flush();
    }

    connector.saveLearningData(false);
    return 0;
}
