#include "hazkey/frontend/server_client.h"

namespace hazkey::frontend {

std::optional<hazkey::ResponseEnvelope> ServerClient::request(
    const char* name, const hazkey::RequestEnvelope& request,
    bool tryConnect) {
    auto response = connection_.transact(request, tryConnect);
    if (response == std::nullopt) {
        hooks_.log(LogLevel::Error,
                   std::string("Error while transacting ") + name + "().");
        return std::nullopt;
    }
    if (response->status() != hazkey::SUCCESS) {
        hooks_.log(LogLevel::Error, std::string(name) +
                                        ": Server returned an error: " +
                                        response->error_message());
        return std::nullopt;
    }
    return response;
}

std::string ServerClient::getComposingText(CharType type,
                                           const std::string& currentPreedit) {
    hazkey::RequestEnvelope req;
    auto props = req.mutable_get_composing_string();
    props->set_char_type(type);
    props->set_current_preedit(currentPreedit);
    auto response = request("getComposingText", req);
    if (!response) return "";
    return response->text();
}

TextWithCursor ServerClient::getComposingHiraganaWithCursor() {
    hazkey::RequestEnvelope req;
    req.mutable_get_hiragana_with_cursor();
    auto response = request("getComposingHiraganaWithCursor", req);
    if (!response) return {};
    if (!response->has_text_with_cursor()) {
        hooks_.log(LogLevel::Error,
                   "getHiraganaWithCursor: Server returned unexpected response");
        return {};
    }
    return {response->text_with_cursor().beforecursosr(),
            response->text_with_cursor().oncursor(),
            response->text_with_cursor().aftercursor()};
}

void ServerClient::inputChar(const std::string& text) {
    hazkey::RequestEnvelope req;
    req.mutable_input_char()->set_text(text);
    request("inputChar", req);
}

void ServerClient::shiftKeyEvent(bool isRelease) {
    hazkey::RequestEnvelope req;
    auto props = req.mutable_modifier_event();
    props->set_event_type(
        isRelease ? hazkey::commands::ModifierEvent_EventType_RELEASE
                  : hazkey::commands::ModifierEvent_EventType_PRESS);
    props->set_mod_type(hazkey::commands::ModifierEvent_ModifierType_SHIFT);
    request("shiftKeyEvent", req);
}

bool ServerClient::currentInputModeIsDirect() {
    hazkey::RequestEnvelope req;
    req.mutable_get_current_input_mode();
    auto response = request("currentInputModeIsDirect", req);
    if (!response) return false;
    return response->current_input_mode_info().input_mode() ==
           hazkey::commands::CurrentInputModeInfo_InputMode_DIRECT;
}

void ServerClient::deleteLeft() {
    hazkey::RequestEnvelope req;
    req.mutable_delete_left();
    request("deleteLeft", req);
}

void ServerClient::deleteRight() {
    hazkey::RequestEnvelope req;
    req.mutable_delete_right();
    request("deleteRight", req);
}

void ServerClient::moveCursor(int offset) {
    hazkey::RequestEnvelope req;
    req.mutable_move_cursor()->set_offset(offset);
    request("moveCursor", req);
}

void ServerClient::setContext(const std::string& context, int anchor) {
    hazkey::RequestEnvelope req;
    auto props = req.mutable_set_context();
    props->set_context(context);
    props->set_anchor(anchor);
    request("setContext", req);
}

void ServerClient::newComposingText() {
    hazkey::RequestEnvelope req;
    req.mutable_new_composing_text();
    request("newComposingText", req);
}

void ServerClient::completePrefix(int index) {
    hazkey::RequestEnvelope req;
    req.mutable_prefix_complete()->set_index(index);
    request("completePrefix", req);
}

void ServerClient::directConversionComplete(CharType charType) {
    hazkey::RequestEnvelope req;
    req.mutable_direct_conversion_complete()->set_char_type(charType);
    request("directConversionComplete", req);
}

void ServerClient::deleteTrailingClause() {
    hazkey::RequestEnvelope req;
    req.mutable_delete_trailing_clause();
    request("deleteTrailingClause", req);
}

std::string ServerClient::completePrefixClauses() {
    hazkey::RequestEnvelope req;
    req.mutable_complete_prefix_clauses();
    auto response = request("completePrefixClauses", req);
    if (!response) return "";
    return response->text();
}

void ServerClient::insertHiragana(const std::string& text) {
    hazkey::RequestEnvelope req;
    req.mutable_insert_hiragana()->set_text(text);
    request("insertHiragana", req);
}

hazkey::commands::CandidatesResult ServerClient::getCandidates(
    bool isSuggest) {
    hazkey::RequestEnvelope req;
    req.mutable_get_candidates()->set_is_suggest(isSuggest);
    auto response = request("getCandidates", req);
    if (!response) return hazkey::commands::CandidatesResult();
    return response->candidates();
}

void ServerClient::saveLearningData(bool tryConnect) {
    hazkey::RequestEnvelope req;
    req.mutable_save_learning_data();
    request("saveLearningData", req, tryConnect);
}

}  // namespace hazkey::frontend
