//! Serves the web player (`web/static`, the WebAssembly core and the ROMs) and relays
//! shared sessions from players to the people watching them.

pub mod relay;
pub mod roms;

use std::path::PathBuf;
use std::sync::Arc;
use std::time::Duration;

use axum::extract::{Path, Query, State, WebSocketUpgrade};
use axum::http::header;
use axum::response::{IntoResponse, Response};
use axum::routing::get;
use axum::Router;
use serde::Deserialize;
use tower_http::services::{ServeDir, ServeFile};

use relay::{Relay, FRAME_MESSAGE_LEN};

/// Where the server finds what it serves.
#[derive(Clone, Debug)]
pub struct Config {
    /// The page: index.html and the files it loads.
    pub static_dir: PathBuf,
    /// The ROMs, with their settings files.
    pub roms_dir: PathBuf,
    /// The compiled core, chip8_wasm.wasm.
    pub wasm: PathBuf,
    /// How long a session waits for its player to reconnect.
    pub grace: Duration,
}

#[derive(Clone)]
struct AppState {
    relay: Arc<Relay>,
    /// The ROM list as JSON, which doesn't change while the server runs.
    roms: Arc<String>,
}

/// The server's routes.
pub fn app(config: &Config) -> Result<Router, String> {
    let state = AppState {
        relay: Relay::new(config.grace),
        roms: Arc::new(
            serde_json::to_string(&roms::load(&config.roms_dir)?).map_err(|e| e.to_string())?,
        ),
    };
    let index = config.static_dir.join("index.html");
    Ok(Router::new()
        .route("/roms.json", get(roms_json))
        .route("/ws/play", get(play))
        .route("/ws/watch/{id}", get(watch))
        .route("/healthz", get(|| async { "ok" }))
        // a shared link opens the page, which watches the session in it
        .route_service("/watch/{id}", ServeFile::new(&index))
        .route_service("/chip8.wasm", ServeFile::new(&config.wasm))
        .nest_service("/roms", ServeDir::new(&config.roms_dir))
        .fallback_service(ServeDir::new(&config.static_dir))
        .layer(axum::middleware::map_response(no_cache))
        .with_state(state))
}

/// The page and ROMs change with each deploy, and they are small, so browsers check for a
/// newer version each time.
async fn no_cache(mut response: Response) -> Response {
    response.headers_mut().insert(
        header::CACHE_CONTROL,
        header::HeaderValue::from_static("no-cache"),
    );
    response
}

async fn roms_json(State(state): State<AppState>) -> impl IntoResponse {
    (
        [(header::CONTENT_TYPE, "application/json")],
        state.roms.as_ref().clone(),
    )
}

#[derive(Deserialize)]
struct Resume {
    id: Option<String>,
    token: Option<String>,
}

async fn play(
    ws: WebSocketUpgrade,
    Query(resume): Query<Resume>,
    State(state): State<AppState>,
) -> impl IntoResponse {
    let resume = resume.id.zip(resume.token);
    ws.max_message_size(FRAME_MESSAGE_LEN.max(1024))
        .on_upgrade(move |socket| state.relay.play(socket, resume))
}

async fn watch(
    ws: WebSocketUpgrade,
    Path(id): Path<String>,
    State(state): State<AppState>,
) -> impl IntoResponse {
    ws.max_message_size(1024)
        .on_upgrade(move |socket| state.relay.watch(socket, id))
}
