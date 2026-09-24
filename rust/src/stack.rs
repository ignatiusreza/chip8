/// Fixed-size call stack holding return addresses.
pub struct Stack {
    values: [u16; Stack::CAPACITY],
    len: usize,
}

impl Stack {
    pub const CAPACITY: usize = 16;

    pub fn new() -> Self {
        Stack {
            values: [0; Self::CAPACITY],
            len: 0,
        }
    }

    /// Pushes a value, returning `false` instead of overflowing.
    pub fn push(&mut self, val: u16) -> bool {
        if self.len == Self::CAPACITY {
            return false;
        }
        self.values[self.len] = val;
        self.len += 1;
        true
    }

    /// Pops a value, returning `None` instead of underflowing.
    pub fn pop(&mut self) -> Option<u16> {
        if self.len == 0 {
            return None;
        }
        self.len -= 1;
        Some(self.values[self.len])
    }

    pub fn peek(&self) -> Option<u16> {
        self.len.checked_sub(1).map(|i| self.values[i])
    }
}

impl Default for Stack {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Tests that popping stack will not underflow.
    #[test]
    fn underflow() {
        let mut s = Stack::new();
        assert_eq!(s.pop(), None);
    }

    // Tests that pushing stack pass the CAPACITY will not overflow.
    #[test]
    fn overflow() {
        let mut s = Stack::new();
        for i in 0..Stack::CAPACITY {
            assert!(s.push(i as u16));
        }
        assert!(!s.push(Stack::CAPACITY as u16));
    }

    // Tests that popping pushed value on stack return correct value.
    #[test]
    fn push_pop() {
        let mut s = Stack::new();
        for i in 0..Stack::CAPACITY {
            assert!(s.push(i as u16));
        }
        assert!(!s.push(Stack::CAPACITY as u16));
        for i in (0..Stack::CAPACITY).rev() {
            assert_eq!(s.peek(), Some(i as u16));
            assert_eq!(s.pop(), Some(i as u16));
        }
        assert_eq!(s.pop(), None);
    }

    // Tests that stack can store large enough value.
    #[test]
    fn max_value() {
        let mut s = Stack::new();
        assert!(s.push(0xfff));
        assert_eq!(s.pop(), Some(0xfff));
    }
}
