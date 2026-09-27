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

Install [CMake](https://cmake.org) 3.16 or newer, a C++17 compiler and SDL 1.2 (look below), then run

    cmake -S . -B build
    cmake --build build

and run the emulator using "build/chip8 ROM", no fancy GUI yet. :P

The code is split in two, like the Rust and Go ports:

- `src/core/` - the CHIP-8 machine (`CPU`, `Display`, `Keypad`, `Stack`), built as the `chip8_core`
  library. It has no SDL dependency, so it can be unit tested on its own.
- `src/frontend/` - the SDL `Graphic`, `Input` and `Sound` classes, and the `Emulator` that wires
  them to the CPU. `src/main.cc` just reads the ROM and runs the `Emulator` loop.


[SDL (Simple DirectMedia Layer)](http://www.libsdl.org/)
-------------------------------------------------------

is used to handle graphics and audio,
the included files under ./include and ./lib are used when compiling on Windows (e.g. with
`cmake -S . -B build -A Win32`, as they are 32-bit), unless the `SDLDIR` environment variable points
to another copy.

For compiling under linux, please use the distribution to install lib sdl

    e.g (on ubuntu) : sudo apt-get install libsdl1.2-dev


[googletest](http://code.google.com/p/googletest/)
-------------------------------------------------

is used as a test framework, granted I included it here just to give it a try, and so far it
only included basic test for the stack.

It is a really good test framework though, so in case you need a test framework for your C++ codes,
give it a try.

The build uses the system googletest if there is one, and downloads it otherwise

    e.g (on ubuntu) : sudo apt-get install libgtest-dev

after compilation, to run the test use :

    ctest --test-dir build

Pass `-DCHIP8_BUILD_TESTS=OFF` to cmake to skip building them.
