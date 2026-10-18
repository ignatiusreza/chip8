#ifndef _CHIP8_FRONTEND_SCREEN_H
#define	_CHIP8_FRONTEND_SCREEN_H

#include <array>
#include "../chip8/display.h"

// The keys held down this frame, as read from a window or terminal.
struct Keys {
  // down state for each CHIP-8 keypad value, 0x0 - 0xF
  std::array<bool, 16> down{};
  // whether Esc was pressed (or the window closed)
  bool quit = false;
};

// Where the display is shown and the keyboard read from: the terminal or a window.
class Screen {
  public:
    virtual ~Screen() = default;

    // the keys held down right now
    virtual Keys keys() = 0;

    // shows the display if it changed
    virtual void draw(Display &display) = 0;
};

#endif	/* _CHIP8_FRONTEND_SCREEN_H */
