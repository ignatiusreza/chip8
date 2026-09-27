chip8 (Rust)
============

A Rust port of the C++ emulator in [`../cpp`](../cpp), keeping the same structure
(CPU, display, keypad) and timing (3 opcodes per ~16ms tick). The call stack is a
plain `Vec`, capped at 16 entries.

The emulator core (`src/lib.rs`) has no windowing or audio dependency, so it can be
unit tested on its own. `src/frontend/` wires it to the outside world, like the C++
`Graphic`, `Input` and `Sound` classes, using pure-Rust crates:
[crossterm](https://crates.io/crates/crossterm) for the terminal display and keyboard,
[minifb](https://crates.io/crates/minifb) for the window display and keyboard, and
[rodio](https://crates.io/crates/rodio) for the beeper. `src/main.rs` just loads the
ROM and runs the `Emulator` loop.

Building
--------

Install a [Rust toolchain](https://rustup.rs). On Linux, audio playback also needs the
ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libasound2-dev pkg-config

then run the emulator with

    cargo run --release -- ROM

which draws in the terminal, or in a separate window with

    cargo run --release -- --display window ROM

and the tests with

    cargo test

Differences from the C++ version
--------------------------------

By default the display is drawn in the terminal, as 64x16 half-block characters inside a
border, which needs a terminal of at least 66x19. Most terminals only report key presses
(repeated while held), not releases, so a key counts as held for 200ms after its last
press. Terminals that support the
[kitty keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/) (kitty,
WezTerm, foot, Ghostty, recent Alacritty...) also report releases, and are used for exact
key state.

The window is scaled 8x (512x256) rather than 10x, since minifb only supports
power-of-two scales. If no audio device is available the emulator runs without sound.
