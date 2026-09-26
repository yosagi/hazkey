#ifndef HAZKEY_FRONTEND_KEYSYMS_H_
#define HAZKEY_FRONTEND_KEYSYMS_H_

#include <cstdint>

// X11 keysym values used by the state machine. fcitx5 (FcitxKey_*) and
// ibus (IBUS_KEY_*) use the same values, so frontends can pass their keysyms
// through unchanged. Defined here to avoid depending on X11 headers.
namespace hazkey::frontend::keysym {

constexpr uint32_t Space = 0x0020;
constexpr uint32_t Digit0 = 0x0030;
constexpr uint32_t Digit1 = 0x0031;
constexpr uint32_t Digit9 = 0x0039;
constexpr uint32_t H = 0x0048;
constexpr uint32_t I = 0x0049;
constexpr uint32_t O = 0x004f;
constexpr uint32_t P = 0x0050;
constexpr uint32_t T = 0x0054;
constexpr uint32_t U = 0x0055;
constexpr uint32_t h = 0x0068;
constexpr uint32_t i = 0x0069;
constexpr uint32_t o = 0x006f;
constexpr uint32_t p = 0x0070;
constexpr uint32_t t = 0x0074;
constexpr uint32_t u = 0x0075;

constexpr uint32_t BackSpace = 0xff08;
constexpr uint32_t Tab = 0xff09;
constexpr uint32_t Return = 0xff0d;
constexpr uint32_t Escape = 0xff1b;
constexpr uint32_t Muhenkan = 0xff22;
constexpr uint32_t Henkan = 0xff23;
constexpr uint32_t Left = 0xff51;
constexpr uint32_t Up = 0xff52;
constexpr uint32_t Right = 0xff53;
constexpr uint32_t Down = 0xff54;
constexpr uint32_t F6 = 0xffc3;
constexpr uint32_t F7 = 0xffc4;
constexpr uint32_t F8 = 0xffc5;
constexpr uint32_t F9 = 0xffc6;
constexpr uint32_t F10 = 0xffc7;
constexpr uint32_t Shift_L = 0xffe1;
constexpr uint32_t Shift_R = 0xffe2;
constexpr uint32_t Delete = 0xffff;

}  // namespace hazkey::frontend::keysym

#endif  // HAZKEY_FRONTEND_KEYSYMS_H_
