chip8 (Rust)
============

A Rust port of the C++ emulator in [`../cpp`](../cpp), keeping the same structure
(CPU, display, keypad) and timing (3 opcodes per ~16ms tick). The call stack is a
plain `Vec`, capped at 16 entries.

The emulator core (`src/lib.rs`) has no windowing or audio dependency, so it can be
unit tested on its own; `src/main.rs` is the frontend, using pure-Rust crates:
[minifb](https://crates.io/crates/minifb) for the window and keyboard, and
[rodio](https://crates.io/crates/rodio) for the beeper.

Building
--------

Install a [Rust toolchain](https://rustup.rs). On Linux, audio playback also needs the
ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libasound2-dev pkg-config

then run the emulator with

    cargo run --release -- ROM

and the tests with

    cargo test

Differences from the C++ version
--------------------------------

The window is scaled 8x (512x256) rather than 10x, since minifb only supports
power-of-two scales. If no audio device is available the emulator runs without sound.
