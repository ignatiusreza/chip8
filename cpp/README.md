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

run the emulator using "build/chip8 ROM" (no fancy GUI yet :P),

and run the tests with

    ctest --test-dir build

If SDL3 or [googletest](https://github.com/google/googletest) isn't installed, CMake downloads and
builds it; for SDL3 on linux that needs its
[build dependencies](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies).

Keys are matched by their position on the keyboard, so the [keypad layout](../README.md#keyboard) is
the same on non-QWERTY keyboards. If no audio device is available the emulator runs without sound.

Code layout
-----------

The code is split in two, like the Rust and Go ports:

- `src/core/` - the CHIP-8 machine (`CPU`, `Display`, `Keypad`, `Stack`), built as the `chip8_core`
  library. It has no SDL dependency, so it can be unit tested on its own.
- `src/frontend/` - the SDL `Graphic`, `Input` and `Sound` classes, and the `Emulator` that wires
  them to the CPU. `src/main.cc` just reads the ROM and runs the `Emulator` loop.

The tests in `test/` cover the stack and the CPU's opcodes; the opcode tests are ported from the
Rust and Go versions, so all three emulators are checked against the same behaviour.
