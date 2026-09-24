chip8
=====

A simple [CHIP-8](http://en.wikipedia.org/wiki/CHIP-8) emulator, implemented in more than one language.

- [`cpp/`](cpp/) - the original C++ implementation (SDL 1.2, googletest)
- [`rust/`](rust/) - a port of the same emulator to Rust (minifb, rodio)

References for the bytecodes used is from :

- http://en.wikipedia.org/wiki/CHIP-8
- http://devernay.free.fr/hacks/chip8/schip.txt

Fixes
-----

These bugs from the original C++ version were fixed while porting to Rust, and then
applied back to the C++ version, so both behave the same:

- `8XYE` sets VF to the shifted-out bit (0 or 1), not `0x80`
- `8XY5` / `8XY7` set VF when there is no borrow, including equal operands
- `8XY4` - `8XYE` write VF after the result, so the flag is kept when X is F
- `0NNN` is skipped instead of stalling the program counter
- `FX0A` resumes on a keypress only, not on a key release
- `FX29` uses only the low nibble of VX, so it always points at a font glyph
- `EX9E` / `EXA1` with VX above 0xF no longer crash
- memory is the full 4KB (`0x1000` bytes), and addresses wrap instead of overflowing
- ROMs larger than the 3.5KB of program memory are rejected instead of overflowing it
- sprites are clipped at the right edge instead of wrapping onto the next row
- the random number generator is seeded, so `CXNN` differs between runs
- the beeper plays an audible 440Hz square wave, for as long as the sound timer runs

Keyboard
--------

Both implementations map the 16-key hex keypad to the same keys, and quit on `Esc`:

    1 Q W E        0 1 2 3
    A S D Z   ->   4 5 6 7
    X C R F        8 9 A B
    V T G B        C D E F
