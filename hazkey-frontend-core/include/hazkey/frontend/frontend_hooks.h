#ifndef HAZKEY_FRONTEND_FRONTEND_HOOKS_H_
#define HAZKEY_FRONTEND_FRONTEND_HOOKS_H_

#include <optional>
#include <string>

namespace hazkey::frontend {

struct SurroundingText {
    std::string text;
    int anchor = 0;
};

// Information the state machine asks from the frontend.
class FrontendHooks {
   public:
    virtual ~FrontendHooks() = default;

    // Text around the cursor in the application, if available.
    virtual std::optional<SurroundingText> surroundingText() = 0;
    // Whether to show the "[Press Tab to Select]" hint.
    virtual bool showTabToSelect() = 0;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_FRONTEND_HOOKS_H_
