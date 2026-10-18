#ifndef _CHIP8_FRONTEND_EMULATOR_H
#define	_CHIP8_FRONTEND_EMULATOR_H

#include <memory>
#include <string>
#include "../core/cpu.h"
#include "options.h"
#include "screen.h"
#include "sound.h"

// A CPU running a ROM, with its display, keypad and beeper hooked up.
class Emulator {
  CPU     _cpu;
  // opened before the screen, as it may log to the terminal
  Sound   _sound;
  std::unique_ptr<Screen> _screen;
  int     _speed;

  public:
    // loads the ROM, then opens the audio device and the screen; throws
    // std::runtime_error if the screen can't be opened
    explicit Emulator(const Options &options);

    // runs the ROM until Esc is pressed or the window is closed
    void run();

  private:
    // runs one ~1/60s frame: timers, then the opcodes (handling input before each), then
    // the screen
    void tick();
};

#endif	/* _CHIP8_FRONTEND_EMULATOR_H */
