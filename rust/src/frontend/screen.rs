use chip8::Display;

use super::Result;

/// The keys held down this frame, as read from a window or terminal.
#[derive(Default)]
pub struct Keys {
    /// Down state for each CHIP-8 keypad value, 0x0 - 0xF.
    pub down: [bool; 16],
    /// Whether Esc was pressed.
    pub quit: bool,
}

/// A frontend that shows the display and reads the keyboard, once per frame.
pub trait Screen {
    /// The keys held down right now.
    fn keys(&mut self) -> Keys;
    /// Shows the display, then waits for the next 1/60s frame.
    fn draw(&mut self, display: &mut Display) -> Result<()>;
}
