chip8
=====

A simple [CHIP-8](http://en.wikipedia.org/wiki/CHIP-8) emulator, implemented in more than one language.

- [`cpp/`](cpp/) - the original C++ implementation (SDL 1.2, googletest)
- [`rust/`](rust/) - a port of the same emulator to Rust (SDL2)

References for the bytecodes used is from :

- http://en.wikipedia.org/wiki/CHIP-8
- http://devernay.free.fr/hacks/chip8/schip.txt

Keyboard
--------

Both implementations map the 16-key hex keypad to the same keys, and quit on `Esc`:

    1 Q W E        0 1 2 3
    A S D Z   ->   4 5 6 7
    X C R F        8 9 A B
    V T G B        C D E F
