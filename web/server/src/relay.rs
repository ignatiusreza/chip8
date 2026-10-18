//! Shared sessions: a player streams what their emulator shows, and anyone with the
//! session's link watches it live. The server runs no emulator, it only keeps each
//! session's latest picture and forwards updates.
//!
//! The player connects to `/ws/play` and is sent `{"type":"session","id","token"}`. It
//! then sends `{"type":"hello","rom","quirks","speed"}` text messages when it starts a
//! ROM, and a binary message each time the display changes: a flags byte (`FLAG_HIRES`,
//! `FLAG_SOUND`) followed by the packed 128x64 frame. It is sent
//! `{"type":"viewers","count"}` as people come and go. If it disconnects, it can resume
//! the session for `grace` by connecting to `/ws/play?id=…&token=…`; after that the
//! session ends.
//!
//! A viewer connects to `/ws/watch/{id}` and is sent the latest hello and frame, then
//! everything the player sends, and `{"type":"ended"}` when the session ends (or at once
//! if there is no such session). Viewers send nothing.

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use axum::body::Bytes;
use axum::extract::ws::{Message, Utf8Bytes, WebSocket};
use futures_util::{SinkExt, StreamExt};
use rand::distributions::{Alphanumeric, DistString};
use serde::{Deserialize, Serialize};
use tokio::sync::{broadcast, watch};

/// The size of a frame message: the flags byte and the packed 128x64 frame.
pub const FRAME_MESSAGE_LEN: usize = 1 + 128 * 64 / 8;
/// Frame message flags.
pub const FLAG_HIRES: u8 = 1;
pub const FLAG_SOUND: u8 = 2;

/// How many sessions and viewers per session the server takes at once.
pub const MAX_SESSIONS: usize = 500;
pub const MAX_VIEWERS: usize = 50;

/// What the player sends when it starts a ROM.
#[derive(Debug, Deserialize, Serialize)]
struct Hello {
    rom: String,
    quirks: String,
    speed: u32,
}

impl Hello {
    fn valid(&self) -> bool {
        let allowed = |s: &str, extra: &str| {
            s.chars()
                .all(|c| c.is_ascii_alphanumeric() || extra.contains(c))
        };
        self.rom.len() <= 64
            && allowed(&self.rom, "-_")
            && self.quirks.len() <= 100
            && allowed(&self.quirks, "-,")
            && (1..=10000).contains(&self.speed)
    }
}

/// What a session forwards to its viewers.
#[derive(Clone, Debug)]
enum Update {
    Hello(Utf8Bytes),
    Frame(Bytes),
    Ended,
}

#[derive(Default)]
struct Latest {
    hello: Option<Utf8Bytes>,
    frame: Option<Bytes>,
    /// Whether the player is connected; a session without one ends after the grace.
    playing: bool,
    /// Counts the player's connections, so an old grace timer leaves a resumed session.
    connection: u64,
    /// Set when the session ends, for viewers who join as it does and miss `Update::Ended`.
    ended: bool,
}

struct Session {
    token: String,
    latest: Mutex<Latest>,
    updates: broadcast::Sender<Update>,
    viewers: watch::Sender<usize>,
}

/// The live sessions, by id.
pub struct Relay {
    sessions: Mutex<HashMap<String, Arc<Session>>>,
    grace: Duration,
}

fn json(value: serde_json::Value) -> Message {
    Message::Text(value.to_string().into())
}

impl Relay {
    /// A relay whose sessions end `grace` after their player disconnects.
    pub fn new(grace: Duration) -> Arc<Self> {
        Arc::new(Relay {
            sessions: Mutex::new(HashMap::new()),
            grace,
        })
    }

    /// Resumes the session `id` if `token` is its own and its player is gone, otherwise
    /// starts a new one, unless there are too many.
    fn start(&self, resume: Option<(String, String)>) -> Option<(String, Arc<Session>, u64)> {
        let mut sessions = self.sessions.lock().unwrap();
        if let Some((id, token)) = resume {
            if let Some(session) = sessions.get(&id) {
                let mut latest = session.latest.lock().unwrap();
                if session.token == token && !latest.playing {
                    latest.playing = true;
                    latest.connection += 1;
                    let connection = latest.connection;
                    drop(latest);
                    return Some((id, session.clone(), connection));
                }
            }
        }
        if sessions.len() >= MAX_SESSIONS {
            return None;
        }
        let id = loop {
            let id = Alphanumeric.sample_string(&mut rand::thread_rng(), 8);
            if !sessions.contains_key(&id) {
                break id;
            }
        };
        let session = Arc::new(Session {
            token: Alphanumeric.sample_string(&mut rand::thread_rng(), 32),
            latest: Mutex::new(Latest {
                playing: true,
                connection: 1,
                ..Latest::default()
            }),
            updates: broadcast::channel(64).0,
            viewers: watch::channel(0).0,
        });
        sessions.insert(id.clone(), session.clone());
        Some((id, session, 1))
    }

    /// Runs a player's connection until it closes, then gives it `grace` to come back.
    pub async fn play(self: Arc<Self>, mut socket: WebSocket, resume: Option<(String, String)>) {
        let Some((id, session, connection)) = self.start(resume) else {
            let _ = socket.send(json(serde_json::json!({"type": "full"}))).await;
            return;
        };
        let hello = serde_json::json!({"type": "session", "id": id, "token": session.token});
        let mut viewers = session.viewers.subscribe();
        viewers.mark_changed();
        if socket.send(json(hello)).await.is_ok() {
            loop {
                tokio::select! {
                    message = socket.recv() => match message {
                        Some(Ok(Message::Text(text))) => {
                            let Ok(hello) = serde_json::from_str::<Hello>(&text) else { break };
                            if !hello.valid() {
                                break;
                            }
                            let mut value = serde_json::to_value(&hello).unwrap();
                            value["type"] = "hello".into();
                            let text: Utf8Bytes = value.to_string().into();
                            session.latest.lock().unwrap().hello = Some(text.clone());
                            let _ = session.updates.send(Update::Hello(text));
                        }
                        Some(Ok(Message::Binary(frame))) => {
                            if frame.len() != FRAME_MESSAGE_LEN {
                                break;
                            }
                            session.latest.lock().unwrap().frame = Some(frame.clone());
                            let _ = session.updates.send(Update::Frame(frame));
                        }
                        Some(Ok(Message::Ping(_) | Message::Pong(_))) => {}
                        _ => break,
                    },
                    Ok(()) = viewers.changed() => {
                        let count = *viewers.borrow_and_update();
                        let message = json(serde_json::json!({"type": "viewers", "count": count}));
                        if socket.send(message).await.is_err() {
                            break;
                        }
                    }
                }
            }
        }

        session.latest.lock().unwrap().playing = false;
        tokio::time::sleep(self.grace).await;
        let mut sessions = self.sessions.lock().unwrap();
        let mut latest = session.latest.lock().unwrap();
        if !latest.playing && latest.connection == connection {
            latest.ended = true;
            drop(latest);
            sessions.remove(&id);
            let _ = session.updates.send(Update::Ended);
        }
    }

    /// Runs a viewer's connection to the session `id` until it closes or the session ends.
    pub async fn watch(self: Arc<Self>, socket: WebSocket, id: String) {
        let (mut sink, mut stream) = socket.split();
        let ended = || json(serde_json::json!({"type": "ended"}));
        let Some(session) = self.sessions.lock().unwrap().get(&id).cloned() else {
            let _ = sink.send(ended()).await;
            return;
        };
        if *session.viewers.borrow() >= MAX_VIEWERS {
            let _ = sink.send(json(serde_json::json!({"type": "full"}))).await;
            return;
        }
        // subscribe before taking the latest picture, so no update falls in between
        let mut updates = session.updates.subscribe();
        let (hello, frame, ended_already) = {
            let latest = session.latest.lock().unwrap();
            (latest.hello.clone(), latest.frame.clone(), latest.ended)
        };
        if ended_already {
            let _ = sink.send(ended()).await;
            return;
        }
        session.viewers.send_modify(|count| *count += 1);

        let mut open = true;
        for message in [hello.map(Message::Text), frame.map(Message::Binary)]
            .into_iter()
            .flatten()
        {
            open = open && sink.send(message).await.is_ok();
        }
        while open {
            tokio::select! {
                update = updates.recv() => {
                    let message = match update {
                        Ok(Update::Hello(text)) => Message::Text(text),
                        Ok(Update::Frame(frame)) => Message::Binary(frame),
                        Ok(Update::Ended) | Err(broadcast::error::RecvError::Closed) => {
                            let _ = sink.send(ended()).await;
                            break;
                        }
                        // too slow to keep up: skip to the latest picture
                        Err(broadcast::error::RecvError::Lagged(_)) => {
                            let Some(frame) = session.latest.lock().unwrap().frame.clone() else { continue };
                            Message::Binary(frame)
                        }
                    };
                    open = sink.send(message).await.is_ok();
                }
                message = stream.next() => {
                    open = matches!(message, Some(Ok(_)));
                }
            }
        }
        session.viewers.send_modify(|count| *count -= 1);
    }
}
