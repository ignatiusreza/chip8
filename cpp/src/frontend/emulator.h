#ifndef _CHIP8_FRONTEND_EMULATOR_H
#define	_CHIP8_FRONTEND_EMULATOR_H

#include "../core/cpu.h"
#include "graphic.h"
#include "input.h"
#include "sound.h"

// A CPU running a ROM, with its display, keypad and beeper hooked up to SDL.
// SDL must be initialized before the Emulator is created.
class Emulator {
  CPU     _cpu;
  Graphic _graphic;
  Sound   _sound;
  Input   _input;

  public:
    // copies the ROM into memory; returns false if it is too large
    bool load(const unsigned char *rom, std::size_t size) { return _cpu.load(rom, size); }

    // runs one ~1/60s frame: timers, then 3 opcodes (handling window events before
    // each), then the screen
    void tick();

    bool isRunning() const { return !_input.quitRequested(); }
};

#endif	/* _CHIP8_FRONTEND_EMULATOR_H */
