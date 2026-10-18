#ifndef _CHIP8_FRONTEND_EMULATOR_H
#define	_CHIP8_FRONTEND_EMULATOR_H

#include <memory>
#include <optional>
#include <string>
#include "../chip8/cpu.h"
#include "args.h"
#include "screen.h"
#include "sound.h"

// A CPU running a ROM, with its display, keypad and beeper hooked up.
class Emulator {
  CPU     _cpu;
  // opened once the ROM is loaded, before the screen as it may log to the terminal
  std::optional<Sound> _sound;
  std::unique_ptr<Screen> _screen;
  int     _speed;
  // set once Esc was pressed, or the window closed
  bool    _quit = false;

  public:
    // loads the ROM and its settings file, then opens the audio device and the screen;
    // throws std::runtime_error if the ROM can't be loaded or the screen opened
    explicit Emulator(const Args &args);

    // runs the ROM until Esc is pressed or the window is closed
    void run();

  private:
    // runs one ~1/60s frame: timers, keyboard, a few opcodes, then the screen
    void tick();
};

#endif	/* _CHIP8_FRONTEND_EMULATOR_H */
