use chip8::Display;
use minifb::{Scale, Window, WindowOptions};

const FPS: usize = 60;
const BLACK: u32 = 0x000000;
const WHITE: u32 = 0xFFFFFF;

/// The emulator window, showing the 64x32 display scaled 8x.
pub struct Graphic {
    window: Window,
    buffer: Vec<u32>,
}

impl Graphic {
    pub fn new(title: &str) -> Result<Self, minifb::Error> {
        let mut window = Window::new(
            title,
            Display::WIDTH,
            Display::HEIGHT,
            WindowOptions {
                scale: Scale::X8,
                ..WindowOptions::default()
            },
        )?;
        window.set_target_fps(FPS);
        Ok(Graphic {
            window,
            buffer: vec![BLACK; Display::BUFF_LENGTH],
        })
    }

    /// The window, which also receives the keyboard input.
    pub fn window(&self) -> &Window {
        &self.window
    }

    pub fn is_open(&self) -> bool {
        self.window.is_open()
    }

    /// Redraws the display if it changed. Either way the window is updated, which
    /// polls input and keeps the loop at 60fps.
    pub fn update(&mut self, display: &mut Display) -> Result<(), minifb::Error> {
        if !display.take_invalidated() {
            self.window.update();
            return Ok(());
        }

        for y in 0..Display::HEIGHT {
            for x in 0..Display::WIDTH {
                self.buffer[x + y * Display::WIDTH] = if display.get(x, y) { WHITE } else { BLACK };
            }
        }
        self.window
            .update_with_buffer(&self.buffer, Display::WIDTH, Display::HEIGHT)
    }
}
