#ifndef _CHIP8_FRONTEND_INPUT_H
#define	_CHIP8_FRONTEND_INPUT_H

#include "SDL.h"
#include "../core/cpu.h"

// Keyboard events from the window, forwarded to the CPU's keypad.
class Input {
  bool _quit;

  public:
    Input() : _quit(false) {}

    // handles the pending window events, reporting key presses and releases to the CPU
    void poll(CPU &cpu);

    // whether Esc was pressed or the window closed
    bool quitRequested() const { return _quit; }

  private:
    // CHIP-8 keypad value for a key on the left side of a QWERTY keyboard, or -1
    static int keypadValue(SDLKey key);
};

#endif	/* _CHIP8_FRONTEND_INPUT_H */
