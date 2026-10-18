chip8 (Rust)
============

A Rust port of the C++ emulator in [`../cpp`](../cpp), keeping the same structure
(CPU, display, keypad) and timing (opcodes run in batches every ~16ms tick). The call stack
is a plain `Vec`, capped at 16 entries.

The emulator core (`src/chip8/`, re-exported by `src/lib.rs`), with the ROM settings file
parser (`src/chip8/settings.rs`), has no windowing or audio dependency, so it can be unit tested on its own, and it is also what
the [web version](../web) compiles to WebAssembly. `src/frontend/` wires it to the outside
world, like the C++ `Graphic`, `Input` and `Sound` classes, using pure-Rust crates:
[crossterm](https://crates.io/crates/crossterm) for the terminal display and keyboard,
[minifb](https://crates.io/crates/minifb) for the window display and keyboard, and
[rodio](https://crates.io/crates/rodio) for the beeper; they are only needed by the `chip8`
binary, behind the default `frontend` feature. `src/frontend/layout.rs` places the game and
its how to play in the terminal, and `src/frontend/options.rs` parses the command line and
reads the ROM and its settings into the `Options` the `Emulator` runs, so `src/main.rs` only
runs the `Emulator`.

Building
--------

Install a [Rust toolchain](https://rustup.rs). On Linux, audio playback also needs the
ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libasound2-dev pkg-config

then run the emulator with

    cargo run --release -- ROM

which draws in the terminal, or in a separate window with

    cargo run --release -- --display window ROM

overriding the [quirks](../README.md#quirks) and [speed](../README.md#speed) a ROM's
[settings](../README.md#rom-settings) ask for with e.g.

    cargo run --release -- --quirks vip --speed 30 ROM

and the tests with

    cargo test

Differences from the C++ version
--------------------------------

By default the display is drawn in the terminal, centred, with the ROM's how to play beside or
below it, and double size in a terminal of at least 130x35; it needs at least 66x19 (see the
[top-level README](../README.md)).
Most terminals only report key presses (repeated while held), not releases, so a key counts
as held for 200ms after its last press. Terminals that support the
[kitty keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/) (kitty,
WezTerm, foot, Ghostty, recent Alacritty...) also report releases, and are used for exact
key state.

The window is 512x256 (low resolution pixels scaled 8x) rather than 640x320, since minifb
only supports power-of-two scales. If no audio device is available the emulator runs without
sound.
