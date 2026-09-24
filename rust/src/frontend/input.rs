use chip8::Cpu;
use minifb::{Key, Window};

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

/// Keyboard state, forwarded to the CPU's keypad.
#[derive(Default)]
pub struct Input {
    keys: [bool; 16],
    quit: bool,
}

impl Input {
    /// Reads the keyboard from the window and reports changed keys to the CPU.
    pub fn update(&mut self, window: &Window, cpu: &mut Cpu) {
        self.quit = window.is_key_down(Key::Escape);

        // only report changes, so FX0A waits for a fresh keypress
        for (k, key) in KEYMAP.iter().enumerate() {
            let down = window.is_key_down(*key);
            if down != self.keys[k] {
                self.keys[k] = down;
                cpu.set_key(k as u8, down);
            }
        }
    }

    /// Whether Esc was pressed.
    pub fn quit_requested(&self) -> bool {
        self.quit
    }
}
