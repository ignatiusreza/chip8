chip8 (Rust)
============

A Rust port of the C++ emulator in [`../cpp`](../cpp), keeping the same structure
(CPU, display, keypad, stack) and timing (3 opcodes per ~16ms tick).

The emulator core (`src/lib.rs`) has no dependency on SDL, so it can be unit tested
on its own; `src/main.rs` is the SDL2 frontend handling the window, keyboard and beeper.

Building
--------

Install a [Rust toolchain](https://rustup.rs) and the SDL2 development library:

    e.g (on ubuntu) : sudo apt-get install libsdl2-dev

then run the emulator with

    cargo run --release -- ROM

and the tests with

    cargo test

Differences from the C++ version
--------------------------------

A few opcode bugs were fixed during the port:

- `8XYE` sets VF to the shifted-out bit (0 or 1), not `0x80`
- `8XY5` / `8XY7` set VF when there is no borrow, including equal operands
- `0NNN` is skipped instead of stalling the program counter
- memory is the full 4KB (`0x1000` bytes), and addresses wrap instead of overflowing
- the beeper plays an audible 440Hz square wave
