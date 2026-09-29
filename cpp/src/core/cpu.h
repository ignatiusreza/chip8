#ifndef _CHIP8_CORE_CPU_H
#define	_CHIP8_CORE_CPU_H

#include <cstddef>
#include "display.h"
#include "keypad.h"
#include "stack.h"

#define MEMORY_SIZE 0x1000
#define PROGRAM_START 0x200
#define FONT_BYTE_LENGTH 5

// The CHIP-8 machine: memory, registers, timers, stack, display and keypad. It has
// no windowing or audio dependency; a frontend drives it, shows its display and
// forwards key presses to it.
class CPU {
  unsigned char V[16], data, DT, ST, flag;
  unsigned char _memory[MEMORY_SIZE], X, Y, _keyBuff;
  unsigned short PC;
  unsigned short opcode, I;
  int temp;
  bool _waitForKey;

  Stack   _stack;
  Display _display;
  Keypad  _keypad;

  void _x8000();
  void _xF000();

  public:
    static constexpr std::size_t MAX_ROM_SIZE = MEMORY_SIZE - PROGRAM_START;

    CPU();

    // copies the ROM into memory at 0x200; returns false if it is larger than MAX_ROM_SIZE
    bool load(const unsigned char *rom, std::size_t size);

    // runs the next opcode, unless waiting for a keypress (FX0A)
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
};

#endif	/* _CHIP8_CORE_CPU_H */
