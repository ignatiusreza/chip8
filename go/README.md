chip8 (Go)
==========

A Go port of the same emulator as [`../cpp`](../cpp) and [`../rust`](../rust), keeping the
same structure (CPU, display, keypad) and timing (opcodes run in batches every 1/60s tick).
The call stack is a plain slice, capped at 16 entries.

The emulator core (package [`chip8`](chip8)), with the ROM settings file parser, has no
windowing or audio dependency, so it can be unit tested on its own. Package [`frontend`](frontend) wires it to the outside
world, like the C++ `Graphic`, `Input` and `Sound` classes, using
[x/term](https://pkg.go.dev/golang.org/x/term) for the terminal display and keyboard,
[Ebitengine](https://ebitengine.org) for the window display and keyboard, and
[oto](https://github.com/ebitengine/oto) for the beeper; it also places the game and its
how to play in the terminal, and parses the command line into the `Args` the `Emulator` is
built from, which reads the ROM and its settings, so `main.go` only runs the `Emulator`.

Building
--------

Install [Go](https://go.dev/dl/) 1.25 or newer. On Linux, Ebitengine also needs the X11
and ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libc6-dev libgl1-mesa-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libxxf86vm-dev libasound2-dev pkg-config

then run the emulator with

    go run . ROM

which draws in the terminal, or in a separate window with

    go run . -display window ROM

overriding the [quirks](../README.md#quirks) and [speed](../README.md#speed) a ROM's
[settings](../README.md#rom-settings) ask for with e.g.

    go run . -quirks vip -speed 30 ROM

and the tests with

    go test ./...

The terminal display is centred, with the ROM's how to play beside or below it, and is double
size in a terminal of at least 130x35; it needs at least 66x19 (see the
[top-level README](../README.md)). Terminals only report
key presses (repeated while held), not releases, so a key counts as held for 200ms after its
last press.

If no audio device is available the emulator runs without sound.
