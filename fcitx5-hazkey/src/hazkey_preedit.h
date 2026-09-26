#ifndef _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_
#define _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_

#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

#include <vector>

#include "hazkey/frontend/output.h"

namespace fcitx {

// convert segments from the state machine to fcitx::Text
Text toFcitxText(const std::vector<hazkey::frontend::Segment> &segments,
                 int caretSegment = -1);

class HazkeyPreedit {
   public:
    HazkeyPreedit(InputContext *ic) : ic_(ic) {}

    void render(const hazkey::frontend::Preedit &preedit);

   private:
    void setPreedit(Text text);

    InputContext *ic_;
};

}  // namespace fcitx

#endif  // _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_
