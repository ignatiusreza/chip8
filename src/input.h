#ifndef _CHIP8_INPUT_H
#define	_CHIP8_INPUT_H

#include <bitset>

class Input {
  std::bitset<0x10> _state;

  public:
    bool key(int keyCode) { return _state.test(keyCode); }
    void set(int keyCode, bool pressed) { _state.set(keyCode, pressed); }
};

#endif	/* _CHIP8_INPUT_H */
