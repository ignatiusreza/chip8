# AGENTS.md

A CHIP-8 and SUPER-CHIP emulator implemented three times, in C++ (`cpp/`, the original),
Rust (`rust/`) and Go (`go/`), plus a web version (`web/`) that compiles the Rust core to
WebAssembly. The top-level `README.md` is the spec for user-visible behaviour (instructions,
quirks and presets, speed, ROM settings files, keyboard, terminal layout); keep it in sync.

## The ports mirror each other

The three native ports share the same structure, behaviour and tests, so a change to the
emulator's behaviour is normally made in **all three**, with the matching test in each:

| Concern | C++ | Rust | Go |
|---|---|---|---|
| CPU, opcodes, quirks | `cpp/src/core/cpu.{h,cc}` | `rust/src/cpu.rs` | `go/chip8/cpu.go` |
| Display, keypad | `cpp/src/core/*.h` | `rust/src/{display,keypad}.rs` | `go/chip8/{display,keypad}.go` |
| ROM settings files | `cpp/src/core/settings.cc` | `rust/src/settings.rs` | `go/chip8/settings.go` |
| Terminal layout | `cpp/src/frontend/layout.cc` | `rust/src/frontend/layout.rs` | `go/frontend/layout.go` |
| Terminal, window, input, sound | `cpp/src/frontend/` (SDL3) | `rust/src/frontend/` (crossterm, minifb, rodio) | `go/frontend/` (x/term, Ebitengine, oto) |
| Command line options | `cpp/src/frontend/options.cc` | `rust/src/frontend/options.rs` | `go/frontend/options.go` |
| Emulator loop | `cpp/src/frontend/emulator.cc` | `rust/src/frontend/emulator.rs` | `go/frontend/emulator.go` |

- The tests are ports of each other, with the same names in each language's style
  (`CPUTest.WaitForKey`, `wait_for_key`, `TestWaitForKey`). When adding or changing a test,
  do it in all three.
- The core has no windowing or audio dependency in every port, so it builds and tests on its
  own; in Rust the frontend crates sit behind the default `frontend` feature, and the core
  builds with `default-features = false` (which is how `web/` uses it).
- The frontend drives the core the same way everywhere: `Emulator::run` (`Run` in Go) loops
  over 1/60s frames, and each frame calls `tick_timers` (which decrements DT/ST, reports
  whether to beep and ends the `vblank` quirk's wait), reads the keys, runs `speed` opcodes
  (`step`), then draws. The `Screen` (terminal or window) reports the keys held down as
  `Keys`, and `Input` passes the ones that changed to `set_key(key, pressed)`. Go has no
  `Screen` type, as Ebitengine runs the window's loop: its window and terminal each call the
  `Emulator`'s `tick` with their `Keys`.
- Terminals mostly report only key presses, so the terminal frontends treat a key as held for
  200ms after its last press and then send a release (Rust uses exact releases when the
  terminal supports the kitty keyboard protocol).
- Options are `--display tui|window`, `--quirks LIST`, `--speed N` (single dash in Go).
  Without them, quirks and speed come from the ROM's `.cfg` settings file next to it, then the
  defaults (`octo`, 15).
- Keep each `main` compact: it only gets the `Options`, builds the `Emulator` from them and
  runs it. Parsing, usage text, reading the ROM and settings, and other setup belong in the
  options module (or the frontend), not in `main`.

## Web

`web/` is its own Cargo workspace (`wasm`, `server`) whose crates depend on `../../rust` by
path, so core changes reach the browser and are also tested by `web/wasm`'s tests.

- `web/wasm` exports plain functions (`load`, `set_key`, `run_frame`, `frame`, `buffer`) with
  no generated glue; `web/static/app.js` only draws, handles input and sound, and talks over
  WebSockets.
- `web/server` (axum) serves the page, the wasm module and `roms/` (with `/roms.json` built
  from the settings files), and relays live-shared games; it runs no emulator. The sharing
  and embedding protocol is documented in `web/server/src/relay.rs`. Shared games live in
  memory, so it is deployed as a single Fly.io machine (`fly.toml`, `web/Dockerfile`).

## Commands

```sh
# C++ (CMake downloads SDL3 / googletest if they aren't installed)
cd cpp && cmake -S . -B build && cmake --build build
./build/chip8 [--display window] [--quirks vip] [--speed 30] ROM
ctest --test-dir build                                  # or ./build/chip8_test
./build/chip8_test --gtest_filter='CPUTest.WaitForKey'  # one test

# Rust
cd rust && cargo run --release -- ROM
cargo test
cargo test wait_for_key                                 # one test

# Go
cd go && go run . ROM
go test ./...
go test ./chip8 -run TestWaitForKey                     # one test

# Web
cd web && rustup target add wasm32-unknown-unknown
cargo build --release -p chip8-wasm --target wasm32-unknown-unknown
cargo run -p chip8-server                               # http://localhost:8080
cargo test
npm install --no-save playwright && npx playwright install chromium
node tests/browser.mjs http://localhost:8080            # against the running server
```

The code is clean under `cargo fmt --check` and `cargo clippy` (in `rust/` and `web/`), and
`gofmt -l` and `go vet ./...` (in `go/`); keep it that way. C++ builds with `-Wall -Wextra`.

## ROMs

`roms/` holds CC0 programs from the Chip8 Community Archive, each with a `.cfg` settings file
(title, author, about, controls, quirks, speed); `roms/README.md` lists them and where their
settings come from. Only add freely redistributable ROMs there.

## Conventions

- Comments are sparse, lowercase-first and explain why; opcode cases carry a short comment
  like `// FX0A (wait for a key press and release, store it to VX)`.
- Commit subjects are short sentences in the imperative or describing the change ("FX0A
  waits for the key to be released, like the COSMAC VIP"), prefixed with the port
  (`C++: `) only when the change touches just one.
