chip8 (C++)
===========

Created in an attempt to relearn C++, what used to be my favorite programming language
before I move on to web development and Ruby on Rails,
and to try what it feels like to make a simple emulator.

References for the bytecodes used is from :

- http://en.wikipedia.org/wiki/CHIP-8
- http://devernay.free.fr/hacks/chip8/schip.txt

Building
--------

Install [CMake](https://cmake.org), a C++17 compiler and [SDL3](https://www.libsdl.org/)

    e.g (on ubuntu 25.04+) : sudo apt-get install cmake g++ libsdl3-dev
    e.g (on macos)         : brew install cmake sdl3

then build with

    cmake -S . -B build
    cmake --build build

run the emulator in the terminal using "build/chip8 ROM", or in a window (the fancy GUI :P)
using "build/chip8 --display window ROM",

and run the tests with

    ctest --test-dir build

If SDL3 or [googletest](https://github.com/google/googletest) isn't installed, CMake downloads and
builds it; for SDL3 on linux that needs its
[build dependencies](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies).

The terminal display is drawn as 64x16 half-block characters inside a border, which needs a
terminal of at least 66x19. Terminals only report key presses (repeated while held), not releases,
so a key counts as held for 200ms after its last press. It needs a POSIX terminal, so on Windows the
window is the default.

In the window, keys are matched by their position on the keyboard, so the
[keypad layout](../README.md#keyboard) is the same on non-QWERTY keyboards. If no audio device is
available the emulator runs without sound.

Code layout
-----------

The code is split in two, like the Rust and Go ports:

- `src/core/` - the CHIP-8 machine (`CPU`, `Display`, `Keypad`, `Stack`), built as the `chip8_core`
  library. It has no SDL dependency, so it can be unit tested on its own.
- `src/frontend/` - the `Emulator` that wires the CPU to a `Screen` and the SDL `Sound`. The
  screen is either the `Terminal`, or a `Window` made of the SDL `Graphic` and `Input` classes.
  `src/main.cc` just parses the options, reads the ROM and runs the `Emulator` loop.

The tests in `test/` cover the stack and the CPU's opcodes; the opcode tests are ported from the
Rust and Go versions, so all three emulators are checked against the same behaviour.
