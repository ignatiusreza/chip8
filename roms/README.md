roms
====

A few CHIP-8 programs to try the emulators with, taken from the
[Chip8 Community Archive](https://github.com/JohnEarnest/chip8Archive). Everything in that
archive, including these ROMs, is released under the
[Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/) "No Rights Reserved"
license, so they can be redistributed freely. The source code for each program lives in the
archive's `src/` directory.

| ROM                   | Title           | Author         | Released   |
|-----------------------|-----------------|----------------|------------|
| `br8kout.ch8`         | Br8kout         | SharpenedSpoon | 2014-09-01 |
| `flightrunner.ch8`    | Flight Runner   | TodPunk        | 2014-11-01 |
| `mini-lights-out.ch8` | Mini Lights Out | tobiasvl       | 2019-10-04 |
| `superpong.ch8`       | Super Pong      | offstatic      | 2021-10-15 |

These were picked because they only use plain CHIP-8 and don't depend on the quirks where these
emulators differ from Octo's defaults (shifting `VY` into `VX`, `FX55`/`FX65` advancing `I`, and
sprites wrapping around the screen edges).

Controls
--------

The games were written in Octo, so their own instructions refer to Octo's keyboard layout. With
the keypad mapping used here (see the top-level README), the keys are:

- **Br8kout** - `Z` / `C` move the paddle left / right.
- **Flight Runner** - `S` / `X` / `Z` / `C` move up / down / left / right.
- **Super Pong** - `S` / `X` move the vertical paddle, `Z` / `C` the horizontal one, and `D`
  resets. Press any key to start.
- **Mini Lights Out** - each key toggles the light in the same spot of the 4x4 grid:

      Q W E V
      A S D T
      Z X C G
      R 1 F B

Run them with, for example:

    go run . ../roms/br8kout.ch8
