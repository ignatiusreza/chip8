chip8 (web)
===========

The emulator in the browser: the [Rust](../rust) core compiled to WebAssembly runs the
game on the player's own machine, and a small Rust server serves the page and relays
shared games to the people watching them.

- [`wasm/`](wasm) - the core as a WebAssembly module, exporting a few plain functions
  (`load`, `set_key`, `run_frame`, `frame`, `buffer`), so the page needs no generated
  glue.
- [`static/`](static) - the page. `app.js` only does what the browser has to: drawing the
  frames on a canvas, the keyboard and a touch keypad, the beeper, and the WebSockets.
- [`server/`](server) - an [axum](https://github.com/tokio-rs/axum) server that serves the
  page, the core and the [ROMs](../roms) (with `/roms.json` listing them from their
  settings files), and relays shared games. It runs no emulator.

Playing and sharing
-------------------

Open the page to play: pick a game, and use the same [keys](../README.md#keyboard) as the
other versions, or the keypad on a touch screen. `/?rom=br8kout` opens a game directly.

**Share live** gives a link, `/watch/<id>`, that anyone can open to watch the game as it
is played, with its how to play next to it. The player's page sends each new picture
(1KB, only when it changes) to the server, which keeps the latest one for people who
join late and forwards it to everyone watching; the player sees how many are watching.
Viewers send nothing. A shared game carries on after a reload or a dropped connection,
for 30 seconds, then ends.

Embedded in another page, the player asks it where it is (`postMessage` of
`{"type":"chip8-ready"}`, answered with `{"type":"chip8-host","url"}`), and then shares
`<that page>?watch=<id>` instead, for the page to open in its frame as `/watch/<id>`; the
[blog](https://erauqssidlroweht.com/projects/chip8) does this. It also posts
`{"type":"chip8-height","height"}` as its height changes, so the page can fit the frame to it. The protocol is described in
[`server/src/relay.rs`](server/src/relay.rs).

Running it
----------

Install a [Rust toolchain](https://rustup.rs) with the WebAssembly target:

    rustup target add wasm32-unknown-unknown

then, from this directory, build the core and start the server:

    cargo build --release -p chip8-wasm --target wasm32-unknown-unknown
    cargo run -p chip8-server

and open http://localhost:8080. Run the tests with

    cargo test

and the browser test, against the running server, with

    npm install --no-save playwright && npx playwright install chromium
    node tests/browser.mjs http://localhost:8080

The server reads `PORT`, `STATIC_DIR`, `ROMS_DIR` and `WASM_PATH` from the environment,
with defaults for running it from here as above.

Deploying to Fly.io
-------------------

[`Dockerfile`](Dockerfile) builds everything into one image, and [`../fly.toml`](../fly.toml)
runs it on [Fly.io](https://fly.io). With [flyctl](https://fly.io/docs/flyctl/install/)
installed and logged in, from the repository root:

    fly apps create <name>          # then set app = "<name>" in fly.toml
    fly deploy --ha=false

Shared games live in the server's memory, so it has to run on a single machine, which is
what `--ha=false` asks for (`fly scale count 1` fixes it if there are more). It stops when
nobody is connected and starts on the next visit, which takes a moment.

To serve it from your own domain, e.g. `chip8.erauqssidlroweht.com`, add a `CNAME` record
pointing it at `<name>.fly.dev`, then

    fly certs add chip8.erauqssidlroweht.com
