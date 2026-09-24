chip8 (Go)
==========

A Go port of the same emulator as [`../cpp`](../cpp) and [`../rust`](../rust), keeping the
same structure (CPU, display, keypad) and timing (3 opcodes per 1/60s tick). The call
stack is a plain slice, capped at 16 entries.

The emulator core (package [`chip8`](chip8)) has no windowing or audio dependency, so it
can be unit tested on its own. Package [`frontend`](frontend) wires it to the outside
world, like the C++ `Graphic`, `Input` and `Sound` classes, using
[Ebitengine](https://ebitengine.org) for the window, keyboard and beeper. `main.go` just
loads the ROM and runs the `Emulator`.

Building
--------

Install [Go](https://go.dev/dl/) 1.25 or newer. On Linux, Ebitengine also needs the X11
and ALSA development headers:

    e.g (on ubuntu) : sudo apt-get install libc6-dev libgl1-mesa-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libxxf86vm-dev libasound2-dev pkg-config

then run the emulator with

    go run . ROM

and the tests with

    go test ./...

If no audio device is available the emulator runs without sound.
