chip8
=====

A simple [CHIP-8](http://en.wikipedia.org/wiki/CHIP-8) emulator, implemented in more than one language.

- [`cpp/`](cpp/) - the original C++ implementation (SDL3, googletest)
- [`rust/`](rust/) - a port of the same emulator to Rust (crossterm, minifb, rodio)
- [`go/`](go/) - a port of the same emulator to Go (x/term, Ebitengine, oto)

All three draw in the terminal they are started from by default, and can open a separate
window instead with `--display window`.

A few freely licensed (CC0) ROMs to try them with are in [`roms/`](roms/).

Quirks
------

CHIP-8 interpreters disagree on a few instructions, and ROMs are written for one behaviour or
the other. By default the emulators behave like SUPER-CHIP (except that `BNNN` jumps relative
to `V0`); `--quirks` (`-quirks` in Go) switches to other behaviours with a comma separated list
of presets and quirks, applied from left to right. A preset sets every quirk, a quirk name
turns that one on:

| Quirk        | Behaviour                                                              |
|--------------|------------------------------------------------------------------------|
| `shift`      | `8XY6`/`8XYE` shift `VY` into `VX`, instead of shifting `VX` in place  |
| `load-store` | `FX55`/`FX65` leave `I` pointing past the last register               |
| `vf-reset`   | `8XY1`/`8XY2`/`8XY3` reset `VF` to 0                                   |
| `jump`       | `BNNN` jumps to `NNN + VX` (`X` being the top nibble), instead of `V0` |
| `wrap`       | sprites wrap around the screen edges, instead of being clipped        |

| Preset  | Quirks                         | For ROMs written for                     |
|---------|--------------------------------|------------------------------------------|
| `none`  | -                              | the default behaviour                    |
| `vip`   | `shift,load-store,vf-reset`    | the original COSMAC VIP interpreter      |
| `schip` | `jump`                         | SUPER-CHIP 1.1                           |
| `octo`  | `shift,load-store,wrap`        | [Octo](https://github.com/JohnEarnest/Octo)'s defaults, like most modern homebrew |

For example `--quirks octo`, or `--quirks vip,wrap`. The original VIP also waits for the next
frame before drawing a sprite, which isn't emulated.

References for the bytecodes used is from :

- http://en.wikipedia.org/wiki/CHIP-8
- http://devernay.free.fr/hacks/chip8/schip.txt

Keyboard
--------

All implementations map the 16-key hex keypad to the same keys, and quit on `Esc`:

    1 Q W E        0 1 2 3
    A S D Z   ->   4 5 6 7
    X C R F        8 9 A B
    V T G B        C D E F
