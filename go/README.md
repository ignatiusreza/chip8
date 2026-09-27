chip8 (Go)
==========

A Go port of the same emulator as [`../cpp`](../cpp) and [`../rust`](../rust), keeping the
same structure (CPU, display, keypad) and timing (3 opcodes per 1/60s tick). The call
stack is a plain slice, capped at 16 entries.

The emulator core (package [`chip8`](chip8)) has no windowing or audio dependency, so it
can be unit tested on its own. Package [`frontend`](frontend) wires it to the outside
world, like the C++ `Graphic`, `Input` and `Sound` classes, using
[x/term](https://pkg.go.dev/golang.org/x/term) for the terminal display and keyboard,
[Ebitengine](https://ebitengine.org) for the window display and keyboard, and
[oto](https://github.com/ebitengine/oto) for the beeper. `main.go` just parses the flags,
loads the ROM and runs the `Emulator`.

Building
--------

Install [Go](https://go.dev/dl/) 1.25 or newer. On Linux, Ebitengine also needs the X11
and ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libc6-dev libgl1-mesa-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libxxf86vm-dev libasound2-dev pkg-config

then run the emulator with

    go run . ROM

which draws in the terminal, or in a separate window with

    go run . -display window ROM

and the tests with

    go test ./...

The terminal display is drawn as 64x16 half-block characters inside a border, which needs
a terminal of at least 66x19. Terminals only report key presses (repeated while held), not
releases, so a key counts as held for 200ms after its last press.

If no audio device is available the emulator runs without sound.
