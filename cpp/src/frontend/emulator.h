#ifndef _CHIP8_FRONTEND_EMULATOR_H
#define	_CHIP8_FRONTEND_EMULATOR_H

#include <memory>
#include <string>
#include "../core/cpu.h"
#include "screen.h"
#include "sound.h"

// Where the display is shown and the keyboard read from.
enum class DisplayMode {
  Terminal, // the terminal the emulator was started from
  Window,   // a separate desktop window (SDL video must be initialized first)
};

// A CPU running a ROM, with its display, keypad and beeper hooked up.
class Emulator {
  CPU     _cpu;
  // opened before the screen, as it may log to the terminal
  Sound   _sound;
  std::unique_ptr<Screen> _screen;

  public:
    // opens the audio device and the screen; throws std::runtime_error if the screen
    // can't be opened
    Emulator(const std::string &title, DisplayMode mode);

    // copies the ROM into memory; returns false if it is too large
    bool load(const unsigned char *rom, std::size_t size) { return _cpu.load(rom, size); }

    // runs one ~1/60s frame: timers, then 3 opcodes (handling input before each), then
    // the screen
    void tick();

    bool isRunning() const { return !_screen->quitRequested(); }
};

#endif	/* _CHIP8_FRONTEND_EMULATOR_H */
