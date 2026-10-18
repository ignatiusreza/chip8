chip8
=====

A simple [CHIP-8](http://en.wikipedia.org/wiki/CHIP-8) and SUPER-CHIP emulator, implemented in more
than one language.

- [`cpp/`](cpp/) - the original C++ implementation (SDL3, googletest)
- [`rust/`](rust/) - a port of the same emulator to Rust (crossterm, minifb, rodio)
- [`go/`](go/) - a port of the same emulator to Go (x/term, Ebitengine, oto)

All three draw in the terminal they are started from by default, and can open a separate
window instead with `--display window`.

A few freely licensed (CC0) ROMs to try them with are in [`roms/`](roms/).

SUPER-CHIP
----------

Besides the original instruction set, the emulators run the
[SUPER-CHIP 1.1](http://devernay.free.fr/hacks/chip8/schip.txt) instructions:

- `00FF` / `00FE` switch to the 128x64 high resolution and back to 64x32, clearing the screen
- `00CN` scrolls down N pixels, `00FB` / `00FC` scroll right / left 4 pixels
- `DXY0` draws a 16x16 sprite, two bytes per row
- `FX30` points `I` at the 8x10 font for the digit in `VX`
- `FX75` / `FX85` save and restore `V0`..`VX` to the user flags (kept until the emulator quits)
- `00FD` exits, which stops the program, leaving the screen as it was

Like in [Octo](https://github.com/JohnEarnest/Octo), scrolling in low resolution moves whole
low resolution pixels, `DXY0` draws 16x16 sprites in either resolution, and a collision sets
`VF` to 1 rather than counting the colliding rows. The window shows both resolutions at the
same size; in the terminal, high resolution is drawn with braille characters, 2x4 pixels to a
character, so it fits the same 66x19 terminal.

Most SUPER-CHIP games expect to run much faster than the original interpreter, and the
emulators only run 3 instructions per frame, so they can be sluggish.

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
