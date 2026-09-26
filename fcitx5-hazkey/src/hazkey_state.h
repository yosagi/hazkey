#ifndef _FCITX5_HAZKEY_HAZKEY_STATE_H_
#define _FCITX5_HAZKEY_HAZKEY_STATE_H_

#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/surroundingtext.h>

#include <optional>

#include "hazkey/frontend/frontend_hooks.h"
#include "hazkey/frontend/state_machine.h"
#include "hazkey_preedit.h"

namespace fcitx {

class HazkeyEngine;

// Connects an fcitx input context to the shared state machine: converts key
// events and shows the resulting preedit, aux text and candidates.
class HazkeyState : public InputContextProperty,
                    public hazkey::frontend::FrontendHooks {
   public:
    HazkeyState(HazkeyEngine* engine, InputContext* ic);

    void keyEvent(KeyEvent& keyEvent);
    // commit the preedit as is and reset (on deactivation)
    void commitAndReset();
    // reset to the initial state
    void reset();

    // FrontendHooks
    std::optional<hazkey::frontend::SurroundingText> surroundingText()
        override;
    bool showTabToSelect() override;

   private:
    // check if the key event is inputable (simple key / kana key) or not
    static bool isInputableEvent(const KeyEvent& keyEvent);
    static hazkey::frontend::KeyEvent toCoreKeyEvent(const KeyEvent& keyEvent);
    // replace the input panel contents with the state machine output
    void apply(const hazkey::frontend::Output& output);
    void applyAuxDown(hazkey::frontend::AuxDown aux);

    // engine
    HazkeyEngine* engine_;
    // fcitx input context pointer
    InputContext* ic_;
    HazkeyPreedit preedit_;
    hazkey::frontend::StateMachine core_;
};

}  // namespace fcitx

#endif  // _FCITX5_HAZKEY_HAZKEY_STATE_H_
