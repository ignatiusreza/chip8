use chip8::Cpu;

/// The keys held down this frame, as read from a window or terminal.
#[derive(Default)]
pub struct Keys {
    /// Down state for each CHIP-8 keypad value, 0x0 - 0xF.
    pub down: [bool; 16],
    /// Whether Esc was pressed.
    pub quit: bool,
}

/// Keyboard state, forwarded to the CPU's keypad.
#[derive(Default)]
pub struct Input {
    keys: [bool; 16],
    quit: bool,
}

impl Input {
    /// Reports keys that changed since the last frame to the CPU.
    pub fn update(&mut self, keys: &Keys, cpu: &mut Cpu) {
        self.quit = keys.quit;

        // only report changes, so FX0A waits for a fresh keypress
        for (k, &down) in keys.down.iter().enumerate() {
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
