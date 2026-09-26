#ifndef FCITX5_HAZKEY_HAZKEY_CANDIDATE_H_
#define FCITX5_HAZKEY_HAZKEY_CANDIDATE_H_

#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/text.h>

#include <string>
#include <vector>

#include "hazkey/frontend/output.h"

namespace fcitx {

const KeyList defaultSelectionKeys = {
    Key{FcitxKey_1}, Key{FcitxKey_2}, Key{FcitxKey_3}, Key{FcitxKey_4},
    Key{FcitxKey_5}, Key{FcitxKey_6}, Key{FcitxKey_7}, Key{FcitxKey_8},
    Key{FcitxKey_9}, Key{FcitxKey_0},
};

class HazkeyCandidateWord : public CandidateWord {
   public:
    HazkeyCandidateWord(const int index, const std::string& text)
        : CandidateWord(Text(text)), index_(index) {}

    // called when the candidate is selected (by pointing device?)
    // calculate the index of the candidate on current page
    // and send key to select the candidate
    void select(InputContext* ic) const override;

   private:
    const int index_;
};

// display-only candidate list. the state machine owns the cursor and paging.
class HazkeyCandidateList : public CommonCandidateList {
   public:
    explicit HazkeyCandidateList(
        const hazkey::frontend::CandidateWindow& window);

    // return the direction of the candidate list
    // currently always vertical
    CandidateLayoutHint layoutHint() const override;
};

}  // namespace fcitx

#endif  // FCITX5_HAZKEY_HAZKEY_CANDIDATE_H_
