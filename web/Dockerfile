# The web player's server, with the WebAssembly core, the page and the ROMs. The build
# context is the repository root, which fly.toml deploys from.

FROM rust:1-slim-trixie AS build
RUN rustup target add wasm32-unknown-unknown
WORKDIR /src
COPY rust rust
COPY web web
WORKDIR /src/web
RUN cargo build --release --locked -p chip8-wasm --target wasm32-unknown-unknown \
 && cargo build --release --locked -p chip8-server

FROM debian:trixie-slim
COPY --from=build /src/web/target/release/chip8-server /usr/local/bin/chip8-server
COPY --from=build /src/web/target/wasm32-unknown-unknown/release/chip8_wasm.wasm /app/chip8.wasm
COPY web/static /app/static
COPY roms /app/roms
ENV PORT=8080 STATIC_DIR=/app/static ROMS_DIR=/app/roms WASM_PATH=/app/chip8.wasm
EXPOSE 8080
USER nobody
CMD ["chip8-server"]
