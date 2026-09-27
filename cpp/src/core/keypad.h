#ifndef _CHIP8_CORE_KEYPAD_H
#define	_CHIP8_CORE_KEYPAD_H

#include <bitset>

// State of the 16-key hexadecimal keypad.
class Keypad {
  std::bitset<0x10> _state;

  public:
    // keys outside 0x0 - 0xF are never pressed (bitset::test would throw)
    bool key(int keyCode) const { return keyCode >= 0 && keyCode < 0x10 && _state.test(keyCode); }
    void set(int keyCode, bool pressed) { if(keyCode >= 0 && keyCode < 0x10) _state.set(keyCode, pressed); }
};

#endif	/* _CHIP8_CORE_KEYPAD_H */
