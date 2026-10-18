#ifndef _CHIP8_CORE_CPU_H
#define	_CHIP8_CORE_CPU_H

#include <cstddef>
#include <string>
#include "display.h"
#include "keypad.h"
#include "stack.h"

#define MEMORY_SIZE 0x1000
#define PROGRAM_START 0x200
#define FONT_BYTE_LENGTH 5
// the SUPER-CHIP 8x10 font for the digits 0-9 (FX30) follows the 4x5 font
#define BIG_FONT_START 0x50
#define BIG_FONT_BYTE_LENGTH 10

// Behaviours where CHIP-8 interpreters disagree. With all of them off (the default) the
// CPU behaves like SUPER-CHIP, except that BNNN jumps relative to V0.
struct Quirks {
  bool shift = false;     // 8XY6/8XYE shift VY into VX, instead of shifting VX in place
  bool loadStore = false; // FX55/FX65 leave I pointing past the last register
  bool vfReset = false;   // 8XY1/8XY2/8XY3 reset VF to 0
  bool jump = false;      // BNNN jumps to NNN + VX (X being the top nibble of NNN), instead of V0
  bool wrap = false;      // sprites wrap around the screen edges, instead of being clipped

  // applies a comma separated list of presets (none, vip, schip, octo), which set every
  // quirk, and quirk names (shift, load-store, vf-reset, jump, wrap), which turn one on,
  // from left to right; returns false and sets `unknown` on a name it doesn't know
  bool parse(const std::string &list, std::string &unknown);
};

// The CHIP-8 machine: memory, registers, timers, stack, display and keypad. It has
// no windowing or audio dependency; a frontend drives it, shows its display and
// forwards key presses to it.
class CPU {
  unsigned char V[16], DT, ST, flag;
  // the SUPER-CHIP RPL user flags (FX75/FX85)
  unsigned char _flags[16];
  unsigned char _memory[MEMORY_SIZE], X, Y, _keyBuff;
  unsigned short PC;
  unsigned short opcode, I;
  int temp;
  bool _waitForKey;
  bool _exited;

  Stack   _stack;
  Display _display;
  Keypad  _keypad;
  Quirks  _quirks;

  void _x0000();
  void _x8000();
  void _xF000();
  void _draw();

  public:
    static constexpr std::size_t MAX_ROM_SIZE = MEMORY_SIZE - PROGRAM_START;

    CPU();

    // copies the ROM into memory at 0x200; returns false if it is larger than MAX_ROM_SIZE
    bool load(const unsigned char *rom, std::size_t size);

    void setQuirks(const Quirks &quirks) { _quirks = quirks; }

    // runs the next opcode, unless waiting for a keypress (FX0A) or the program exited (00FD)
    void step();

    // counts both timers down, once per 1/60s; returns whether the beeper should play
    bool tickTimers();

    // presses or releases a keypad key, 0x0 - 0xF
    void setKey(int key, bool pressed);

    Display &display() { return _display; }

    // read-only views of the machine state, for tests and debugging
    unsigned char v(int x) const { return V[x & 0xF]; }
    unsigned short pc() const { return PC; }
    unsigned short index() const { return I; }
    unsigned char memory(int addr) const { return _memory[addr & (MEMORY_SIZE - 1)]; }
    int stackSize() const { return _stack.size(); }
    bool waitingForKey() const { return _waitForKey; }
    bool exited() const { return _exited; }
};

#endif	/* _CHIP8_CORE_CPU_H */
