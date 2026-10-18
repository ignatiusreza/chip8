/// Monochrome frame buffer, 64x32 in low resolution or 128x64 in the SUPER-CHIP high
/// resolution. The frame is always kept at 128x64, with each low resolution pixel
/// covering 2x2 frame pixels, so frontends can draw it the same way in either mode.
pub struct Display {
    frame: [bool; Display::FRAME_LENGTH],
    hires: bool,
    invalidated: bool,
}

impl Display {
    pub const FRAME_WIDTH: usize = 128;
    pub const FRAME_HEIGHT: usize = 64;
    pub const FRAME_LENGTH: usize = Self::FRAME_WIDTH * Self::FRAME_HEIGHT;

    pub fn new() -> Self {
        Display {
            frame: [false; Self::FRAME_LENGTH],
            hires: false,
            invalidated: true,
        }
    }

    pub fn hires(&self) -> bool {
        self.hires
    }

    /// Switches the resolution, clearing the screen.
    pub fn set_hires(&mut self, hires: bool) {
        self.hires = hires;
        self.clear();
    }

    /// The width in pixels of the current resolution.
    pub fn width(&self) -> usize {
        Self::FRAME_WIDTH / self.scale()
    }

    /// The height in pixels of the current resolution.
    pub fn height(&self) -> usize {
        Self::FRAME_HEIGHT / self.scale()
    }

    pub fn clear(&mut self) {
        self.frame = [false; Self::FRAME_LENGTH];
        self.invalidated = true;
    }

    /// Returns the pixel at (x, y); off-screen pixels read as unset.
    pub fn get(&self, x: usize, y: usize) -> bool {
        self.on_screen(x, y) && self.pixel(x * self.scale(), y * self.scale())
    }

    /// Toggles the pixel at (x, y); off-screen pixels are ignored.
    pub fn flip(&mut self, x: usize, y: usize) {
        if !self.on_screen(x, y) {
            return;
        }
        let scale = self.scale();
        for dy in 0..scale {
            for dx in 0..scale {
                let i = x * scale + dx + (y * scale + dy) * Self::FRAME_WIDTH;
                self.frame[i] = !self.frame[i];
            }
        }
        self.invalidated = true;
    }

    /// Scrolls the screen down by n pixels of the current resolution; the pixels
    /// scrolled in are unset.
    pub fn scroll_down(&mut self, n: usize) {
        self.scroll(0, n as isize);
    }

    /// Scrolls the screen left by n pixels of the current resolution.
    pub fn scroll_left(&mut self, n: usize) {
        self.scroll(-(n as isize), 0);
    }

    /// Scrolls the screen right by n pixels of the current resolution.
    pub fn scroll_right(&mut self, n: usize) {
        self.scroll(n as isize, 0);
    }

    /// A pixel of the 128x64 frame, for frontends.
    pub fn pixel(&self, x: usize, y: usize) -> bool {
        self.frame[x + y * Self::FRAME_WIDTH]
    }

    /// Returns true once after the buffer has changed, so the frontend only
    /// redraws when needed.
    pub fn take_invalidated(&mut self) -> bool {
        std::mem::replace(&mut self.invalidated, false)
    }

    fn scale(&self) -> usize {
        if self.hires {
            1
        } else {
            2
        }
    }

    fn on_screen(&self, x: usize, y: usize) -> bool {
        x < self.width() && y < self.height()
    }

    fn scroll(&mut self, dx: isize, dy: isize) {
        let scale = self.scale() as isize;
        let (dx, dy) = (dx * scale, dy * scale);
        let mut scrolled = [false; Self::FRAME_LENGTH];
        for y in 0..Self::FRAME_HEIGHT {
            for x in 0..Self::FRAME_WIDTH {
                let (from_x, from_y) = (x as isize - dx, y as isize - dy);
                if (0..Self::FRAME_WIDTH as isize).contains(&from_x)
                    && (0..Self::FRAME_HEIGHT as isize).contains(&from_y)
                {
                    scrolled[x + y * Self::FRAME_WIDTH] =
                        self.pixel(from_x as usize, from_y as usize);
                }
            }
        }
        self.frame = scrolled;
        self.invalidated = true;
    }
}

impl Default for Display {
    fn default() -> Self {
        Self::new()
    }
}
