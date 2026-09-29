#ifndef _CHIP8_FRONTEND_INPUT_H
#define	_CHIP8_FRONTEND_INPUT_H

#include <SDL3/SDL.h>
#include "../core/cpu.h"

// Keyboard events from the window, forwarded to the CPU's keypad.
class Input {
  bool _quit = false;
  bool _exposed = false;

  public:
    // handles the pending window events, reporting key presses and releases to the CPU
    void poll(CPU &cpu);

    // whether Esc was pressed or the window closed
    bool quitRequested() const { return _quit; }

    // returns true once after the window needs redrawing
    bool takeExposed() { bool exposed = _exposed; _exposed = false; return exposed; }

  private:
    // CHIP-8 keypad value for a key on the left side of the keyboard, or -1. Keys are
    // matched by position, so the layout is the same on non-QWERTY keyboards.
    static int keypadValue(SDL_Scancode key);
};

#endif	/* _CHIP8_FRONTEND_INPUT_H */
