//! Wires the backend-free core to a screen (a terminal or a window), keyboard and
//! speaker.

mod emulator;
mod input;
mod layout;
mod options;
mod screen;
mod sound;
mod terminal;
mod window;

pub use emulator::Emulator;
pub use options::{DisplayMode, Options};

pub type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;
