#ifndef _CHIP8_FRONTEND_SCREEN_H
#define	_CHIP8_FRONTEND_SCREEN_H

#include "../core/cpu.h"

// Where the display is shown and the keyboard read from: the terminal or a window.
class Screen {
  public:
    virtual ~Screen() = default;

    // handles pending input, reporting key presses and releases to the CPU
    virtual void poll(CPU &cpu) = 0;

    // whether Esc was pressed (or the window closed)
    virtual bool quitRequested() const = 0;

    // shows the display if it changed
    virtual void draw(Display &display) = 0;
};

#endif	/* _CHIP8_FRONTEND_SCREEN_H */
