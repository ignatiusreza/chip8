use crate::{Display, Keypad, Stack};

pub const MEMORY_SIZE: usize = 0x1000;
pub const PROGRAM_START: u16 = 0x200;
const FONT_BYTE_LENGTH: u16 = 5;

// store font data at 0x000 - 0x050
const FONT: [u8; 80] = [
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
];

#[derive(Debug, PartialEq, Eq)]
pub struct RomTooLarge(pub usize);

impl std::fmt::Display for RomTooLarge {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let max = MEMORY_SIZE - PROGRAM_START as usize;
        write!(
            f,
            "ROM is {} bytes, but at most {} fit in memory",
            self.0, max
        )
    }
}

impl std::error::Error for RomTooLarge {}

pub struct Cpu {
    v: [u8; 16],
    i: u16,
    pc: u16,
    dt: u8,
    st: u8,
    memory: [u8; MEMORY_SIZE],
    stack: Stack,
    display: Display,
    keypad: Keypad,
    /// Register waiting for a keypress (FX0A); execution halts until it's set.
    wait_for_key: Option<usize>,
    rng: u32,
}

impl Cpu {
    pub fn new() -> Self {
        let seed = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.subsec_nanos())
            .unwrap_or(0);
        Self::with_seed(seed)
    }

    pub fn with_seed(seed: u32) -> Self {
        let mut memory = [0; MEMORY_SIZE];
        memory[..FONT.len()].copy_from_slice(&FONT);
        Cpu {
            v: [0; 16],
            i: 0,
            pc: PROGRAM_START,
            dt: 0,
            st: 0,
            memory,
            stack: Stack::new(),
            display: Display::new(),
            keypad: Keypad::default(),
            wait_for_key: None,
            // xorshift must not start at zero
            rng: seed | 1,
        }
    }

    pub fn load(&mut self, rom: &[u8]) -> Result<(), RomTooLarge> {
        let start = PROGRAM_START as usize;
        if rom.len() > MEMORY_SIZE - start {
            return Err(RomTooLarge(rom.len()));
        }
        self.memory[start..start + rom.len()].copy_from_slice(rom);
        self.pc = PROGRAM_START;
        Ok(())
    }

    pub fn display(&self) -> &Display {
        &self.display
    }

    pub fn display_mut(&mut self) -> &mut Display {
        &mut self.display
    }

    /// Decrements the delay and sound timers; call at 60Hz. Returns true
    /// while the sound timer is active and a beep should be played.
    pub fn tick_timers(&mut self) -> bool {
        self.dt = self.dt.saturating_sub(1);
        if self.st > 0 {
            self.st -= 1;
            return true;
        }
        false
    }

    pub fn set_key(&mut self, key: u8, pressed: bool) {
        self.keypad.set(key, pressed);
        if pressed {
            if let Some(x) = self.wait_for_key.take() {
                self.v[x] = key;
            }
        }
    }

    /// Fetches and executes a single opcode.
    pub fn step(&mut self) {
        // we're waiting for a keypress, don't advance..
        if self.wait_for_key.is_some() {
            return;
        }

        let pc = self.pc as usize;
        let opcode = u16::from_be_bytes([self.memory[pc & 0xFFF], self.memory[(pc + 1) & 0xFFF]]);
        self.pc = self.pc.wrapping_add(2) & 0xFFF;
        self.execute(opcode);
    }

    fn execute(&mut self, opcode: u16) {
        let x = ((opcode & 0x0F00) >> 8) as usize;
        let y = ((opcode & 0x00F0) >> 4) as usize;
        let nnn = opcode & 0x0FFF;
        let nn = (opcode & 0x00FF) as u8;
        let n = (opcode & 0x000F) as u8;

        match opcode & 0xF000 {
            0x0000 => match opcode {
                0x00E0 => self.display.clear(), // clear screen
                0x00EE => self.pc = self.stack.pop().unwrap_or(PROGRAM_START), // return from a subroutine
                _ => {} // 0NNN (machine code routine) is unsupported
            },
            0x1000 => self.pc = nnn, // 1NNN (jump to NNN)
            0x2000 => {
                // 2NNN (call subroutine at NNN)
                self.stack.push(self.pc);
                self.pc = nnn;
            }
            0x3000 => self.skip_if(self.v[x] == nn), // 3XNN (skip next inst if VX == NN)
            0x4000 => self.skip_if(self.v[x] != nn), // 4XNN (skip next inst if VX != NN)
            0x5000 => self.skip_if(self.v[x] == self.v[y]), // 5XY0 (skip next inst if VX == VY)
            0x6000 => self.v[x] = nn,                // 6XNN (VX = NN)
            0x7000 => self.v[x] = self.v[x].wrapping_add(nn), // 7XNN (VX += NN)
            0x8000 => self.x8000(x, y, n),
            0x9000 => self.skip_if(self.v[x] != self.v[y]), // 9XY0 (skip next inst if VX != VY)
            0xA000 => self.i = nnn,                         // ANNN (I = NNN)
            0xB000 => self.pc = (nnn + self.v[0] as u16) & 0xFFF, // BNNN (PC = NNN + V0)
            0xC000 => self.v[x] = self.random() & nn,       // CXNN (VX = random() & NN)
            0xD000 => self.draw(x, y, n),
            0xE000 => match nn {
                0x9E => self.skip_if(self.keypad.key(self.v[x])), // EX9E (skip if key VX is pressed)
                0xA1 => self.skip_if(!self.keypad.key(self.v[x])), // EXA1 (skip if key VX isn't pressed)
                _ => {}
            },
            0xF000 => self.xf000(x, nn),
            _ => unreachable!(),
        }
    }

    fn x8000(&mut self, x: usize, y: usize, n: u8) {
        let (vx, vy) = (self.v[x], self.v[y]);
        match n {
            0x0 => self.v[x] = vy,  // 8XY0 (VX = VY)
            0x1 => self.v[x] |= vy, // 8XY1 (VX = VX | VY)
            0x2 => self.v[x] &= vy, // 8XY2 (VX = VX & VY)
            0x3 => self.v[x] ^= vy, // 8XY3 (VX = VX ^ VY)
            0x4 => {
                // 8XY4 VX += VY, with VF = carry
                let (res, carry) = vx.overflowing_add(vy);
                self.v[x] = res;
                self.v[0xF] = carry as u8;
            }
            0x5 => {
                // 8XY5 VX -= VY, with VF = NOT borrow
                self.v[x] = vx.wrapping_sub(vy);
                self.v[0xF] = (vx >= vy) as u8;
            }
            0x6 => {
                // 8XY6 VX >> 1, VF = LSB
                self.v[x] = vx >> 1;
                self.v[0xF] = vx & 0x1;
            }
            0x7 => {
                // 8XY7 VX = VY - VX, with VF = NOT borrow
                self.v[x] = vy.wrapping_sub(vx);
                self.v[0xF] = (vy >= vx) as u8;
            }
            0xE => {
                // 8XYE VX << 1, VF = MSB
                self.v[x] = vx << 1;
                self.v[0xF] = vx >> 7;
            }
            _ => {}
        }
    }

    fn xf000(&mut self, x: usize, nn: u8) {
        let i = self.i as usize;
        match nn {
            0x07 => self.v[x] = self.dt,                            // FX07 (VX = DT)
            0x0A => self.wait_for_key = Some(x), // FX0A (wait for keypress and store it to VX)
            0x15 => self.dt = self.v[x],         // FX15 (DT = VX)
            0x18 => self.st = self.v[x],         // FX18 (ST = VX)
            0x1E => self.i = self.i.wrapping_add(self.v[x] as u16), // FX1E (I += VX)
            // FX29 (I = location of font for value of VX)
            0x29 => self.i = (self.v[x] & 0xF) as u16 * FONT_BYTE_LENGTH,
            0x33 => {
                // FX33 (I[0..2] = BCD(VX))
                let vx = self.v[x];
                self.memory[i & 0xFFF] = vx / 100;
                self.memory[(i + 1) & 0xFFF] = (vx / 10) % 10;
                self.memory[(i + 2) & 0xFFF] = vx % 10;
            }
            0x55 => {
                // FX55 (I[0..X] = V0..VX)
                for r in 0..=x {
                    self.memory[(i + r) & 0xFFF] = self.v[r];
                }
            }
            0x65 => {
                // FX65 (V0..VX = I[0..X])
                for r in 0..=x {
                    self.v[r] = self.memory[(i + r) & 0xFFF];
                }
            }
            _ => {}
        }
    }

    /// DXYN: draw an 8xN sprite from memory at I to (VX, VY), VF = collision.
    fn draw(&mut self, x: usize, y: usize, n: u8) {
        let (vx, vy) = (self.v[x] as usize, self.v[y] as usize);
        self.v[0xF] = 0;
        for yline in 0..n as usize {
            let data = self.memory[(self.i as usize + yline) & 0xFFF];
            for xpix in 0..8 {
                if data & (0x80 >> xpix) != 0 {
                    let (px, py) = (vx + xpix, vy + yline);
                    if self.display.get(px, py) {
                        self.v[0xF] = 1;
                    }
                    self.display.flip(px, py);
                }
            }
        }
    }

    fn skip_if(&mut self, cond: bool) {
        if cond {
            self.pc = self.pc.wrapping_add(2) & 0xFFF;
        }
    }

    // xorshift32
    fn random(&mut self) -> u8 {
        self.rng ^= self.rng << 13;
        self.rng ^= self.rng >> 17;
        self.rng ^= self.rng << 5;
        self.rng as u8
    }
}

impl Default for Cpu {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn run(program: &[u16]) -> Cpu {
        let rom: Vec<u8> = program.iter().flat_map(|op| op.to_be_bytes()).collect();
        let mut cpu = Cpu::with_seed(42);
        cpu.load(&rom).unwrap();
        for _ in program {
            cpu.step();
        }
        cpu
    }

    #[test]
    fn rejects_oversized_rom() {
        let mut cpu = Cpu::new();
        assert_eq!(cpu.load(&[0; 0xE01]), Err(RomTooLarge(0xE01)));
    }

    #[test]
    fn set_and_add() {
        let cpu = run(&[0x6A12, 0x7AFF]);
        assert_eq!(cpu.v[0xA], 0x11); // wraps
    }

    #[test]
    fn add_with_carry() {
        let cpu = run(&[0x60F0, 0x6120, 0x8014]);
        assert_eq!(cpu.v[0], 0x10);
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn sub_with_not_borrow() {
        let cpu = run(&[0x6005, 0x6105, 0x8015]);
        assert_eq!(cpu.v[0], 0);
        assert_eq!(cpu.v[0xF], 1);

        let cpu = run(&[0x6001, 0x6102, 0x8015]);
        assert_eq!(cpu.v[0], 0xFF);
        assert_eq!(cpu.v[0xF], 0);
    }

    #[test]
    fn shifts_set_vf_to_shifted_out_bit() {
        let cpu = run(&[0x6081, 0x800E]);
        assert_eq!(cpu.v[0], 0x02);
        assert_eq!(cpu.v[0xF], 1);

        let cpu = run(&[0x6003, 0x8006]);
        assert_eq!(cpu.v[0], 0x01);
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn skip_next_instruction() {
        // V1 = 1 is skipped, so the last step runs past the program
        let cpu = run(&[0x6007, 0x3007, 0x6101, 0x6201]);
        assert_eq!(cpu.pc, 0x20A);
        assert_eq!(cpu.v[1], 0);
        assert_eq!(cpu.v[2], 1);
    }

    #[test]
    fn call_and_return() {
        let mut cpu = Cpu::with_seed(1);
        // 0x200: call 0x206; 0x202: V0 = 1; 0x206: return
        cpu.load(&[0x22, 0x06, 0x60, 0x01, 0x00, 0x00, 0x00, 0xEE])
            .unwrap();
        cpu.step();
        assert_eq!(cpu.pc, 0x206);
        cpu.step();
        assert_eq!(cpu.pc, 0x202);
        cpu.step();
        assert_eq!(cpu.v[0], 1);
    }

    #[test]
    fn bcd() {
        let cpu = run(&[0x60FE, 0xA300, 0xF033]);
        assert_eq!(&cpu.memory[0x300..0x303], &[2, 5, 4]);
    }

    #[test]
    fn store_and_load_registers() {
        let mut cpu = run(&[0x6001, 0x6102, 0x6203, 0xA300, 0xF255]);
        assert_eq!(&cpu.memory[0x300..0x303], &[1, 2, 3]);
        cpu.v = [0; 16];
        cpu.execute(0xF165);
        assert_eq!(&cpu.v[..3], &[1, 2, 0]);
    }

    #[test]
    fn draw_detects_collision() {
        // draw font "0" at (0, 0) twice
        let cpu = run(&[0x6000, 0xF029, 0xD005]);
        assert!(cpu.display.get(0, 0));
        assert_eq!(cpu.v[0xF], 0);

        let cpu = run(&[0x6000, 0xF029, 0xD005, 0xD005]);
        assert!(!cpu.display.get(0, 0));
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn wait_for_key() {
        let mut cpu = run(&[0xF30A, 0x6001]);
        assert_eq!(cpu.pc, 0x202); // halted on the instruction after FX0A
        assert_eq!(cpu.v[0], 0);
        cpu.set_key(0xB, true);
        assert_eq!(cpu.v[3], 0xB);
        cpu.step();
        assert_eq!(cpu.v[0], 1);
    }

    #[test]
    fn key_skips() {
        let mut cpu = Cpu::with_seed(1);
        cpu.load(&[0x60, 0x05, 0xE0, 0x9E]).unwrap();
        cpu.set_key(5, true);
        cpu.step();
        cpu.step();
        assert_eq!(cpu.pc, 0x206);
    }

    #[test]
    fn timers() {
        let mut cpu = run(&[0x6002, 0xF015, 0xF018]);
        assert!(cpu.tick_timers());
        assert!(cpu.tick_timers());
        assert!(!cpu.tick_timers());
        cpu.execute(0xF107);
        assert_eq!(cpu.v[1], 0);
    }
}
