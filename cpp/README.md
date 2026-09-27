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

Install [CMake](https://cmake.org) 3.16 or newer, a C++17 compiler and the build dependencies for SDL3 (look below), then run

    cmake -S . -B build
    cmake --build build

and run the emulator using "build/chip8 ROM", no fancy GUI yet. :P

The code is split in two, like the Rust and Go ports:

- `src/core/` - the CHIP-8 machine (`CPU`, `Display`, `Keypad`, `Stack`), built as the `chip8_core`
  library. It has no SDL dependency, so it can be unit tested on its own.
- `src/frontend/` - the SDL `Graphic`, `Input` and `Sound` classes, and the `Emulator` that wires
  them to the CPU. `src/main.cc` just reads the ROM and runs the `Emulator` loop.


[SDL (Simple DirectMedia Layer)](https://www.libsdl.org/)
--------------------------------------------------------

version 3 is used to handle graphics and audio. The build uses the system SDL3 if there is one, and
otherwise downloads it and builds it along with the emulator, on Windows and macOS as well.

To build SDL3 under linux, please install the development packages for X11/Wayland and audio,
the full list is in [SDL's README-linux](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies)

    e.g (on ubuntu) : sudo apt-get install libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev libdecor-0-dev libegl1-mesa-dev libgles2-mesa-dev libdrm-dev libgbm-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev libudev-dev

Keys are matched by their position on the keyboard, so the [keypad layout](../README.md#keyboard) is the same on
non-QWERTY keyboards. If no audio device is available the emulator runs without sound.


[googletest](http://code.google.com/p/googletest/)
-------------------------------------------------

is used as a test framework, granted I included it here just to give it a try. The tests cover the
stack and the CPU's opcodes; the opcode tests are ported from the Rust and Go versions, so all three
emulators are checked against the same behaviour.

It is a really good test framework though, so in case you need a test framework for your C++ codes,
give it a try.

The build uses the system googletest if there is one, and downloads it otherwise

    e.g (on ubuntu) : sudo apt-get install libgtest-dev

after compilation, to run the test use :

    ctest --test-dir build

Pass `-DCHIP8_BUILD_TESTS=OFF` to cmake to skip building them.
