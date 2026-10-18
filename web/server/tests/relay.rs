//! Runs the server on a free port and checks sharing and watching over real WebSockets.

use std::path::PathBuf;
use std::time::Duration;

use chip8_server::relay::FRAME_MESSAGE_LEN;
use chip8_server::{app, Config};
use futures_util::{SinkExt, StreamExt};
use serde_json::Value;
use tokio::net::TcpStream;
use tokio_tungstenite::tungstenite::Message;
use tokio_tungstenite::{connect_async, MaybeTlsStream, WebSocketStream};

type Socket = WebSocketStream<MaybeTlsStream<TcpStream>>;

const GRACE: Duration = Duration::from_millis(300);

async fn server() -> String {
    let web = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..");
    let config = Config {
        static_dir: web.join("static"),
        roms_dir: web.join("../roms"),
        wasm: web.join("missing.wasm"),
        grace: GRACE,
    };
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let address = listener.local_addr().unwrap();
    let app = app(&config).unwrap();
    tokio::spawn(async move { axum::serve(listener, app).await.unwrap() });
    format!("ws://{address}")
}

async fn connect(url: String) -> Socket {
    connect_async(url).await.unwrap().0
}

/// The next message, as JSON if it is text.
async fn next(socket: &mut Socket) -> Message {
    tokio::time::timeout(Duration::from_secs(2), socket.next())
        .await
        .expect("a message in time")
        .expect("an open socket")
        .unwrap()
}

async fn next_json(socket: &mut Socket) -> Value {
    match next(socket).await {
        Message::Text(text) => serde_json::from_str(&text).unwrap(),
        other => panic!("expected text, got {other:?}"),
    }
}

fn frame(fill: u8) -> Message {
    let mut frame = vec![fill; FRAME_MESSAGE_LEN];
    frame[0] = 1; // high resolution
    Message::Binary(frame.into())
}

fn hello() -> Message {
    Message::Text(r#"{"type":"hello","rom":"br8kout","quirks":"octo","speed":7}"#.into())
}

#[tokio::test]
async fn a_viewer_watches_a_shared_session() {
    let url = server().await;
    let mut player = connect(format!("{url}/ws/play")).await;
    let session = next_json(&mut player).await;
    assert_eq!(session["type"], "session");
    let id = session["id"].as_str().unwrap().to_string();
    assert_eq!(id.len(), 8);
    assert_eq!(next_json(&mut player).await["count"], 0);

    player.send(hello()).await.unwrap();
    player.send(frame(0xAA)).await.unwrap();
    tokio::time::sleep(Duration::from_millis(50)).await;

    // a viewer joining late gets the latest hello and frame at once
    let mut viewer = connect(format!("{url}/ws/watch/{id}")).await;
    let hello = next_json(&mut viewer).await;
    assert_eq!(
        (
            hello["type"].as_str(),
            hello["rom"].as_str(),
            hello["speed"].as_u64()
        ),
        (Some("hello"), Some("br8kout"), Some(7))
    );
    assert_eq!(next(&mut viewer).await, frame(0xAA));
    assert_eq!(next_json(&mut player).await["count"], 1);

    // then each new frame
    player.send(frame(0x55)).await.unwrap();
    assert_eq!(next(&mut viewer).await, frame(0x55));

    // a second viewer, then the first leaves
    let mut second = connect(format!("{url}/ws/watch/{id}")).await;
    assert_eq!(next_json(&mut second).await["type"], "hello");
    assert_eq!(next_json(&mut player).await["count"], 2);
    viewer.close(None).await.unwrap();
    assert_eq!(next_json(&mut player).await["count"], 1);

    // when the player leaves for good, the session ends
    drop(player);
    assert_eq!(next(&mut second).await, frame(0x55));
    assert_eq!(next_json(&mut second).await["type"], "ended");
    let mut late = connect(format!("{url}/ws/watch/{id}")).await;
    assert_eq!(next_json(&mut late).await["type"], "ended");
}

#[tokio::test]
async fn a_player_resumes_within_the_grace() {
    let url = server().await;
    let mut player = connect(format!("{url}/ws/play")).await;
    let session = next_json(&mut player).await;
    let (id, token) = (
        session["id"].as_str().unwrap(),
        session["token"].as_str().unwrap(),
    );
    let mut viewer = connect(format!("{url}/ws/watch/{id}")).await;
    drop(player);

    // a wrong token starts a new session instead
    let mut other = connect(format!("{url}/ws/play?id={id}&token=nope")).await;
    assert_ne!(next_json(&mut other).await["id"], id);

    let mut player = connect(format!("{url}/ws/play?id={id}&token={token}")).await;
    assert_eq!(next_json(&mut player).await["id"], id);
    tokio::time::sleep(GRACE * 2).await;
    player.send(frame(0x0F)).await.unwrap();
    assert_eq!(next(&mut viewer).await, frame(0x0F));
}

#[tokio::test]
async fn bad_messages_close_the_player() {
    let url = server().await;
    for bad in [
        Message::Binary(vec![0; 10].into()),
        Message::Text(r#"{"type":"hello","rom":"<script>","quirks":"octo","speed":7}"#.into()),
        Message::Text(r#"{"type":"hello","rom":"br8kout","quirks":"octo","speed":0}"#.into()),
        Message::Text("not json".into()),
    ] {
        let mut player = connect(format!("{url}/ws/play")).await;
        next_json(&mut player).await;
        next_json(&mut player).await;
        player.send(bad).await.unwrap();
        let closed = tokio::time::timeout(Duration::from_secs(2), async {
            loop {
                match player.next().await {
                    None | Some(Err(_)) | Some(Ok(Message::Close(_))) => return,
                    Some(Ok(_)) => {}
                }
            }
        })
        .await;
        assert!(closed.is_ok());
    }
}

#[tokio::test]
async fn watching_an_unknown_session_ends_at_once() {
    let url = server().await;
    let mut viewer = connect(format!("{url}/ws/watch/nosuchid")).await;
    assert_eq!(next_json(&mut viewer).await["type"], "ended");
}
