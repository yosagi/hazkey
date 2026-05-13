#ifndef _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_
#define _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_

#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

namespace fcitx {
class HazkeyPreedit {
   public:
    HazkeyPreedit(InputContext *ic) : ic_(ic) {}

    std::string text() const;
    void setSimplePreeditHighlighted(const std::string &text);
    void setSimplePreedit(const std::string &text);
    void setSimplePreeditWithFurigana(const std::string &text,
                                      int stablePrefixLen,
                                      const std::string &furigana);
    void setMultiSegmentPreedit(std::vector<std::string> &texts, int cursor);
    void setPreedit(Text text);
    void commitPreedit();
    void clear();

   private:
    InputContext *ic_;
    std::string commitText_;
};

}  // namespace fcitx

#endif  // _FCITX5_HAZKEY_HAZKEY_PREEDIT_H_
