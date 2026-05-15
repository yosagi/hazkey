#ifndef HAZKEY_EMACS_MOZC_OUTPUT_BUILDER_H_
#define HAZKEY_EMACS_MOZC_OUTPUT_BUILDER_H_

#include <cstdint>
#include <string>

#include "hazkey_emacs_state.h"

namespace MozcOutputBuilder {

std::string buildGreeting();
std::string buildResponse(uint32_t eventId, uint32_t sessionId,
                          const OutputData& output);
std::string buildCreateSessionResponse(uint32_t eventId, uint32_t sessionId);
std::string buildDeleteSessionResponse(uint32_t eventId, uint32_t sessionId);

}  // namespace MozcOutputBuilder

#endif
