/// State of the 16-key hexadecimal keypad.
#[derive(Default)]
pub struct Keypad {
    state: u16,
}

impl Keypad {
    pub fn key(&self, key: u8) -> bool {
        key < 16 && self.state & (1 << key) != 0
    }

    pub fn set(&mut self, key: u8, pressed: bool) {
        if key >= 16 {
            return;
        }
        if pressed {
            self.state |= 1 << key;
        } else {
            self.state &= !(1 << key);
        }
    }
}
