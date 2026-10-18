//! Wires the backend-free core to a screen (a terminal or a window), keyboard and
//! speaker.

mod args;
mod emulator;
mod layout;
mod screen;
mod sound;
mod terminal;
mod window;

pub use args::{Args, DisplayMode};
pub use emulator::Emulator;

pub type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;
