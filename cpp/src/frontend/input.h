#ifndef _CHIP8_FRONTEND_INPUT_H
#define	_CHIP8_FRONTEND_INPUT_H

#include <array>
#include "../chip8/cpu.h"

// The keys held down this frame, as read from a window or terminal.
struct Keys {
  // down state for each CHIP-8 keypad value, 0x0 - 0xF
  std::array<bool, 16> down{};
  // whether Esc was pressed (or the window closed)
  bool quit = false;
};

// Keyboard state, forwarded to the CPU's keypad.
class Input {
  std::array<bool, 16> _keys{};
  bool _quit = false;

  public:
    // reports keys that changed since the last frame to the CPU
    void update(const Keys &keys, CPU &cpu);

    // whether Esc was pressed (or the window closed)
    bool quitRequested() const { return _quit; }
};

#endif	/* _CHIP8_FRONTEND_INPUT_H */
