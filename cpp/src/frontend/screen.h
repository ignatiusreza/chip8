#ifndef _CHIP8_FRONTEND_SCREEN_H
#define	_CHIP8_FRONTEND_SCREEN_H

#include "../chip8/display.h"
#include "input.h"

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
