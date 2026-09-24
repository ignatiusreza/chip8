/// Monochrome 64x32 frame buffer.
pub struct Display {
    buff: [bool; Display::BUFF_LENGTH],
    invalidated: bool,
}

impl Display {
    pub const WIDTH: usize = 64;
    pub const HEIGHT: usize = 32;
    pub const BUFF_LENGTH: usize = Self::WIDTH * Self::HEIGHT;

    pub fn new() -> Self {
        Display {
            buff: [false; Self::BUFF_LENGTH],
            invalidated: true,
        }
    }

    pub fn clear(&mut self) {
        self.buff = [false; Self::BUFF_LENGTH];
        self.invalidated = true;
    }

    /// Returns the pixel at (x, y); off-screen pixels read as unset.
    pub fn get(&self, x: usize, y: usize) -> bool {
        Self::index(x, y).is_some_and(|i| self.buff[i])
    }

    /// Toggles the pixel at (x, y); off-screen pixels are ignored.
    pub fn flip(&mut self, x: usize, y: usize) {
        if let Some(i) = Self::index(x, y) {
            self.buff[i] = !self.buff[i];
            self.invalidated = true;
        }
    }

    /// Returns true once after the buffer has changed, so the frontend only
    /// redraws when needed.
    pub fn take_invalidated(&mut self) -> bool {
        std::mem::replace(&mut self.invalidated, false)
    }

    fn index(x: usize, y: usize) -> Option<usize> {
        (x < Self::WIDTH && y < Self::HEIGHT).then(|| x + y * Self::WIDTH)
    }
}

impl Default for Display {
    fn default() -> Self {
        Self::new()
    }
}
