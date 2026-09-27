use chip8::Display;
use minifb::{Key, Scale, Window as MinifbWindow, WindowOptions};

use super::input::Keys;
use super::{Result, Screen};

const FPS: usize = 60;
const BLACK: u32 = 0x000000;
const WHITE: u32 = 0xFFFFFF;

// CHIP-8 keypad value for each index, mapped onto the left side of a QWERTY keyboard
const KEYMAP: [Key; 16] = [
    Key::Key1, // 0
    Key::Q,    // 1
    Key::W,    // 2
    Key::E,    // 3
    Key::A,    // 4
    Key::S,    // 5
    Key::D,    // 6
    Key::Z,    // 7
    Key::X,    // 8
    Key::C,    // 9
    Key::R,    // A
    Key::F,    // B
    Key::V,    // C
    Key::T,    // D
    Key::G,    // E
    Key::B,    // F
];

/// A desktop window, showing the 64x32 display scaled 8x and reading the keyboard.
pub struct Window {
    window: MinifbWindow,
    buffer: Vec<u32>,
}

impl Window {
    pub fn new(title: &str) -> Result<Self> {
        let mut window = MinifbWindow::new(
            title,
            Display::WIDTH,
            Display::HEIGHT,
            WindowOptions {
                scale: Scale::X8,
                ..WindowOptions::default()
            },
        )?;
        window.set_target_fps(FPS);
        Ok(Window {
            window,
            buffer: vec![BLACK; Display::BUFF_LENGTH],
        })
    }
}

impl Screen for Window {
    fn keys(&mut self) -> Keys {
        Keys {
            down: KEYMAP.map(|key| self.window.is_key_down(key)),
            quit: !self.window.is_open() || self.window.is_key_down(Key::Escape),
        }
    }

    /// Redraws the display if it changed. Either way the window is updated, which
    /// polls input and keeps the loop at 60fps.
    fn draw(&mut self, display: &mut Display) -> Result<()> {
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
            .update_with_buffer(&self.buffer, Display::WIDTH, Display::HEIGHT)?;
        Ok(())
    }
}
