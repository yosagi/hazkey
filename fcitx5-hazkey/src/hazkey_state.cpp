#include "hazkey_state.h"

#include <fcitx-utils/key.h>
#include <fcitx-utils/log.h>

#include <memory>
#include <string>

#include "fcitx-utils/keysym.h"
#include "hazkey_candidate.h"
#include "hazkey_engine.h"

namespace fcitx {

HazkeyState::HazkeyState(HazkeyEngine* engine, InputContext* ic)
    : engine_(engine),
      ic_(ic),
      preedit_(HazkeyPreedit(ic)),
      core_(engine->server(), *this) {}

bool HazkeyState::isInputableEvent(const KeyEvent& event) {
    auto key = event.key();
    if (key.check(FcitxKey_space) || key.isSimple() ||
        Key::keySymToUTF8(key.sym()).size() > 1 ||
        (key.sym() >= 0x04a1 && key.sym() <= 0x04df)) {
        // 0x04a1 - 0x04dd is the range of kana keys
        return true;
    }
    return false;
}

// The keysym comes from the normalized key, but the modifiers come from the
// raw key: normalization drops Shift from keys like Shift+BackSpace.
hazkey::frontend::KeyEvent HazkeyState::toCoreKeyEvent(const KeyEvent& event) {
    namespace mod = hazkey::frontend::mod;

    hazkey::frontend::KeyEvent coreEvent;
    coreEvent.sym = event.key().sym();
    auto states = event.rawKey().states();
    if (states.test(KeyState::Shift)) coreEvent.mods |= mod::Shift;
    if (states.test(KeyState::Ctrl)) coreEvent.mods |= mod::Ctrl;
    if (states.test(KeyState::Alt)) coreEvent.mods |= mod::Alt;
    if (isInputableEvent(event)) {
        coreEvent.text = Key::keySymToUTF8(event.key().sym());
    }
    coreEvent.isRelease = event.isRelease();
    return coreEvent;
}

void HazkeyState::keyEvent(KeyEvent& event) {
    FCITX_DEBUG() << "HazkeyState keyEvent";

    auto output = core_.keyEvent(toCoreKeyEvent(event));
    if (output.result == hazkey::frontend::KeyResult::Ignored) {
        // key releases and a lone Shift press only update the lower aux
        // text. resetting the whole panel here makes some clients (wezterm)
        // drop the preedit highlight.
        applyAuxDown(output.auxDown);
    } else {
        apply(output);
    }

    switch (output.result) {
        case hazkey::frontend::KeyResult::Consumed:
            event.filterAndAccept();
            break;
        case hazkey::frontend::KeyResult::PassToApplication:
            event.filter();
            break;
        case hazkey::frontend::KeyResult::Ignored:
            break;
    }
}

void HazkeyState::commitAndReset() { apply(core_.commitAndReset()); }

void HazkeyState::reset() {
    FCITX_DEBUG() << "HazkeyState reset";
    apply(core_.reset());
}

void HazkeyState::apply(const hazkey::frontend::Output& output) {
    auto& panel = ic_->inputPanel();
    panel.reset();

    if (!output.commit.empty()) {
        ic_->commitString(output.commit);
    }

    preedit_.render(output.preedit);

    if (!output.auxUp.empty()) {
        panel.setAuxUp(toFcitxText(output.auxUp));
    }

    applyAuxDown(output.auxDown);

    if (output.candidates.visible) {
        panel.setCandidateList(
            std::make_unique<HazkeyCandidateList>(output.candidates));
    }
}

void HazkeyState::applyAuxDown(hazkey::frontend::AuxDown aux) {
    // appending fcitx::Text is supported only >= 5.1.9
    auto auxDown = Text();
    switch (aux) {
        case hazkey::frontend::AuxDown::TabToSelect:
            auxDown.append(std::string(_("[Press Tab to Select]")));
            break;
        case hazkey::frontend::AuxDown::DirectInput:
            auxDown.append(std::string(_("[Direct Input]")));
            break;
        case hazkey::frontend::AuxDown::None:
            break;
    }
    ic_->inputPanel().setAuxDown(auxDown);
}

/// FrontendHooks

std::optional<hazkey::frontend::SurroundingText>
HazkeyState::surroundingText() {
    if (ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) &&
        ic_->surroundingText().isValid()) {
        auto& surroundingText = ic_->surroundingText();
        return hazkey::frontend::SurroundingText{
            surroundingText.text(), static_cast<int>(surroundingText.anchor())};
    }
    return std::nullopt;
}

bool HazkeyState::showTabToSelect() {
    return engine_->config().showTabToSelect.value();
}

}  // namespace fcitx
