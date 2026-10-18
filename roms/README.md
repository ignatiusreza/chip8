roms
====

CHIP-8 and SUPER-CHIP programs to try the emulators with, taken from the
[Chip8 Community Archive](https://github.com/JohnEarnest/chip8Archive). Everything in that
archive, including these ROMs, is released under the
[Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/) "No Rights Reserved"
license, so they can be redistributed freely. The source code for each program lives in the
archive's `src/` directory.

Settings
--------

Each ROM that doesn't run well with the default [quirks](../README.md#quirks) (`octo`) and
[speed](../README.md#speed) (15 opcodes per frame) has a settings file next to it, with the
same name and a `.cfg` extension, which the emulators read when they load the ROM:

    # Dino Run by Andrezinrc, from the Chip8 Community Archive
    quirks = vip

The settings come from each program's entry in the archive, which records the Octo quirk flags
and speed it was made for. The table shows them, with `-` for the default.

| ROM | Title | Author | Platform | Quirks | Speed | Description |
|-----|-------|--------|----------|--------|-------|-------------|
| `br8kout.ch8` | Br8kout | SharpenedSpoon | CHIP-8 | - | 7 | You can't even imagine how amazing this breakout clone is! |
| `caveexplorer.ch8` | Cave Explorer | JohnEarnest | CHIP-8 | - | 20 | Do you have what it takes to be a Cave Explorer? |
| `chipcross.ch8` | Chipcross | buffi | SUPER-CHIP | - | 20 | 16 levels of Picross implemented in CHIP8 |
| `danm8ku.ch8` | Danm8ku | buffi | CHIP-8 | - | 1000 | Bullet hell for the Chip-8! Survive 8 waves of exciting bullet patterns! |
| `dinorun.ch8` | Dino Run | Andrezinrc | CHIP-8 | vip | - | An endless runner game for Chip-8 |
| `down8.ch8` | down8 | tinaun | CHIP-8 | - | - | you fall down an endless hole and then you die! |
| `eaty.ch8` | Eaty The Alien | JohnEarnest | SUPER-CHIP | - | 200 | An incredible intergalactic adventure |
| `flightrunner.ch8` | Flight Runner | TodPunk | CHIP-8 | - | - | A simple flight runner for the Chip8 |
| `fuse.ch8` | Fuse | JohnEarnest | CHIP-8 | - | - | A minimalist reinterpretation of the classic Pipe Dream |
| `mastermind.ch8` | Mastermind | WilliamDonnelly | CHIP-8 | - | 20 | The classic puzzle game of deduction |
| `mini-lights-out.ch8` | Mini Lights Out | tobiasvl | CHIP-8 | - | 7 | Turn the lights out |
| `mrworm.ch8` | Mr Worm | rmmh | SUPER-CHIP | - | 20 | A snake game with more degrees of freedom, inspired by the TI-89 game |
| `outlaw.ch8` | Outlaw | JohnEarnest | CHIP-8 | - | - | An adaptation of the Atari 2600 game "Outlaw" for the Chip8 |
| `piper.ch8` | Piper | Aeris, JordanMecom, LillianWang | CHIP-8 | - | 200 | Play as a small bird to collect goodies and avoid waves |
| `rockto.ch8` | Rockto | SupSuper | SUPER-CHIP | - | - | An adaptation of "Boulder Dash" for the Chip8 |
| `snake.ch8` | Snake | TimoTriisa | SUPER-CHIP | - | - | Classic Snake game without walls |
| `snek.ch8` | Snek | JohnEarnest | CHIP-8 | - | 1000 | Minimalist Snake in 65 bytes |
| `spaceracer.ch8` | Space Racer | WilliamDonnelly | CHIP-8 | - | 20 | Based on the classic 2-player 'Space Race' video game |
| `octogon.ch8` | Super Octogon | JohnEarnest | SUPER-CHIP | schip | 200 | Eight fantastic sides, and eight awesome angles |
| `superpong.ch8` | Super Pong | offstatic | CHIP-8 | - | 30 | A fast pong-like |
| `sweetcopter.ch8` | Sw8 Copter | JohnEarnest | SUPER-CHIP | - | 100 | An adaptation of "Swing Copters" for the Super Chip8 |
| `binding.ch8` | The Binding of COSMAC | buffi | SUPER-CHIP | schip,shift,load-store,jump-v0 | 1000 | Simplified Isaac demake for SCHIP |
| `tick.ch8` | Tick | StanislavDidus | CHIP-8 | vf-reset | 12 | Rhythm one button game for Chip-8 |
| `wdl.ch8` | WDL | JohnEarnest | CHIP-8 | vip | - | That word game, but shorter |

Controls
--------

The games were written in [Octo](https://github.com/JohnEarnest/Octo), which uses the same
[keyboard layout](../README.md#keyboard) as these emulators, so the keys in their own
instructions work as they are. Most move with `W` `A` `S` `D` and act with `E`. For example:

- **Br8kout** - `A` / `D` move the paddle left / right.
- **Flight Runner** - `W` / `S` / `A` / `D` move up / down / left / right.
- **Super Pong** - `W` / `S` move the vertical paddle, `A` / `D` the horizontal one, and `E`
  resets. Press any key to start.
- **Mini Lights Out** - each of `1`-`4`, `Q`-`R`, `A`-`F` and `Z`-`V` toggles the light in the
  same spot of the 4x4 grid.

Run them with, for example:

    go run . ../roms/br8kout.ch8
