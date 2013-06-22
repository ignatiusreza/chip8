chip8
=====

Created in an attempt to relearn C++, what used to be my favorite programming language
before I move on to web development and Ruby on Rails,
and to try what it feels like to make a simple emulator.

References for the bytecodes used is from :

    http://en.wikipedia.org/wiki/CHIP-8
    http://devernay.free.fr/hacks/chip8/schip.txt

For compilation, install the required dependency (SDL - look below), then run make

after compilation run the emulation using "chip8 ROM", no fancy GUI yet. :P


SDL (Simple DirectMedia Layer) - http://www.libsdl.org/
=======================================================

is used to handle graphics and audio,
the included files under ./include and ./lib is for compiling in windows using VisualStudio

For compiling under linux, please use the distribution to install lib sdl

    e.g (on ubuntu) : sudo apt-get install libsdl1.2-dev


googletest - http://code.google.com/p/googletest/
=================================================

is used as a test framework, granted I included it here just to give it a try, and so far it
only included basic test for the stack.

It is a really good test framework though, so in case you need a test framework for your C++ codes,
give it a try.

after compilation, to run the test use :

    LD_LIBRARY_PATH=./test/lib ./chip8_test
