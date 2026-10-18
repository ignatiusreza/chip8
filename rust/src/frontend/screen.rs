use chip8::Display;

use super::input::Keys;
use super::Result;

/// A frontend that shows the display and reads the keyboard, once per frame.
pub trait Screen {
    /// The keys held down right now.
    fn keys(&mut self) -> Keys;
    /// Shows the display, then waits for the next 1/60s frame.
    fn draw(&mut self, display: &mut Display) -> Result<()>;
}
