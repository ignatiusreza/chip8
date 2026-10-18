use chip8::Display;
use minifb::{Key, Scale, Window as MinifbWindow, WindowOptions};

use super::input::Keys;
use super::screen::Screen;
use super::Result;

const FPS: usize = 60;
const BLACK: u32 = 0x000000;
const WHITE: u32 = 0xFFFFFF;

// key for each CHIP-8 keypad value, laid out like the COSMAC
// VIP keypad on the left of a QWERTY keyboard:
//   1 2 3 4        1 2 3 C
//   Q W E R   ->   4 5 6 D
//   A S D F        7 8 9 E
//   Z X C V        A 0 B F
// so W A S D are 5 7 8 9, the up, left, down and right of most games
const KEYMAP: [Key; 16] = [
    Key::X,    // 0
    Key::Key1, // 1
    Key::Key2, // 2
    Key::Key3, // 3
    Key::Q,    // 4
    Key::W,    // 5
    Key::E,    // 6
    Key::A,    // 7
    Key::S,    // 8
    Key::D,    // 9
    Key::Z,    // A
    Key::C,    // B
    Key::Key4, // C
    Key::R,    // D
    Key::F,    // E
    Key::V,    // F
];

/// A desktop window, showing the 128x64 frame scaled 4x (so low resolution pixels are
/// scaled 8x) and reading the keyboard.
pub struct Window {
    window: MinifbWindow,
    buffer: Vec<u32>,
}

impl Window {
    pub fn new(title: &str) -> Result<Self> {
        let mut window = MinifbWindow::new(
            title,
            Display::FRAME_WIDTH,
            Display::FRAME_HEIGHT,
            WindowOptions {
                scale: Scale::X4,
                ..WindowOptions::default()
            },
        )?;
        window.set_target_fps(FPS);
        Ok(Window {
            window,
            buffer: vec![BLACK; Display::FRAME_LENGTH],
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

        for y in 0..Display::FRAME_HEIGHT {
            for x in 0..Display::FRAME_WIDTH {
                self.buffer[x + y * Display::FRAME_WIDTH] =
                    if display.pixel(x, y) { WHITE } else { BLACK };
            }
        }
        self.window.update_with_buffer(
            &self.buffer,
            Display::FRAME_WIDTH,
            Display::FRAME_HEIGHT,
        )?;
        Ok(())
    }
}
