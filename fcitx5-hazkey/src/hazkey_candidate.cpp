#include "hazkey_candidate.h"

namespace fcitx {

/// CandidateWord

void HazkeyCandidateWord::select(InputContext* ic) const {
    FCITX_UNUSED(ic);
    // TODO: reinplement in cleaner way
    // KeyEvent keyEvent(ic, Key(FcitxKey_Return));
    // keyEvent = KeyEvent(
    //     ic, Key(KeySym(FcitxKey_1 + (index_ % ic->inputPanel() )),
    //             KeyState::Alt));
    // ic->keyEvent(keyEvent);
}

/// CandidateList

HazkeyCandidateList::HazkeyCandidateList(
    const hazkey::frontend::CandidateWindow& window)
    : CommonCandidateList() {
    // CandidateWord needs to know their own index
    int i = 0;
    for (const auto& candidate : window.items) {
        append(std::make_unique<HazkeyCandidateWord>(i, candidate.text));
        i++;
    }
    setSelectionKey(defaultSelectionKeys);
    setPageSize(window.pageSize);
    // setGlobalCursorIndex() does not move the page, so set the page first.
    // the cursor goes after it: setPage() carries the old cursor over to the
    // new page. an unfocused list is always on the first page.
    if (window.focused()) {
        if (window.page > 0 && window.page < totalPages()) {
            setPage(window.page);
        }
        setGlobalCursorIndex(window.cursor);
    }
}

CandidateLayoutHint HazkeyCandidateList::layoutHint() const {
    return CandidateLayoutHint::Vertical;
}

}  // namespace fcitx
