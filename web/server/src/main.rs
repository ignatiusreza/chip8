//! Runs the web player's server. Its paths come from the environment, with defaults for
//! running it from `web/` after building the core (see web/README.md):
//!
//! - `PORT`, default 8080
//! - `STATIC_DIR`, default `static`
//! - `ROMS_DIR`, default `../roms`
//! - `WASM_PATH`, default `target/wasm32-unknown-unknown/release/chip8_wasm.wasm`

use std::path::PathBuf;
use std::time::Duration;

use chip8_server::{app, Config};

#[tokio::main]
async fn main() -> Result<(), String> {
    let var =
        |name: &str, default: &str| std::env::var(name).unwrap_or_else(|_| default.to_string());
    let config = Config {
        static_dir: PathBuf::from(var("STATIC_DIR", "static")),
        roms_dir: PathBuf::from(var("ROMS_DIR", "../roms")),
        wasm: PathBuf::from(var(
            "WASM_PATH",
            "target/wasm32-unknown-unknown/release/chip8_wasm.wasm",
        )),
        grace: Duration::from_secs(30),
    };
    let port = var("PORT", "8080");
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{port}"))
        .await
        .map_err(|e| format!("Could not listen on port {port}: {e}"))?;
    println!("Listening on http://localhost:{port}");
    axum::serve(listener, app(&config)?)
        .with_graceful_shutdown(async {
            let _ = tokio::signal::ctrl_c().await;
        })
        .await
        .map_err(|e| e.to_string())
}
