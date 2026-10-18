use super::{Display, Keypad};

pub const MEMORY_SIZE: usize = 0x1000;
pub const PROGRAM_START: u16 = 0x200;
pub const STACK_DEPTH: usize = 16;
const FONT_BYTE_LENGTH: u16 = 5;
/// The SUPER-CHIP 8x10 font for the digits 0-9 (FX30) follows the 4x5 font.
pub const BIG_FONT_START: u16 = 0x50;
pub const BIG_FONT_BYTE_LENGTH: u16 = 10;

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

// and the SUPER-CHIP big font at 0x050 - 0x0B4
const BIG_FONT: [u8; 100] = [
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // 0
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
    0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // 9
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

/// Behaviours where CHIP-8 interpreters disagree. With all of them off (the default)
/// the CPU behaves like SUPER-CHIP 1.1; each quirk turns on another interpreter's
/// behaviour.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Quirks {
    /// 8XY6/8XYE shift VY into VX, instead of shifting VX in place.
    pub shift: bool,
    /// FX55/FX65 leave I pointing past the last register.
    pub load_store: bool,
    /// 8XY1/8XY2/8XY3 reset VF to 0.
    pub vf_reset: bool,
    /// BNNN jumps to NNN + V0, instead of XNN + VX (X being the top nibble).
    pub jump_v0: bool,
    /// Sprites wrap around the screen edges, instead of being clipped.
    pub wrap: bool,
    /// DXYN waits for the next frame, so at most one sprite is drawn per frame.
    pub vblank: bool,
}

impl Quirks {
    /// Applies a comma separated list of presets (octo, vip, schip), which set every
    /// quirk, and quirk names (shift, load-store, vf-reset, jump-v0, wrap, vblank),
    /// which turn one on, from left to right.
    pub fn apply(self, list: &str) -> Result<Self, String> {
        let mut quirks = self;
        for name in list.split(',') {
            match name {
                "octo" => {
                    quirks = Quirks {
                        shift: true,
                        load_store: true,
                        wrap: true,
                        jump_v0: true,
                        ..Quirks::default()
                    }
                }
                "vip" => {
                    quirks = Quirks {
                        shift: true,
                        load_store: true,
                        vf_reset: true,
                        vblank: true,
                        jump_v0: true,
                        ..Quirks::default()
                    }
                }
                "schip" => quirks = Quirks::default(),
                "shift" => quirks.shift = true,
                "load-store" => quirks.load_store = true,
                "vf-reset" => quirks.vf_reset = true,
                "jump-v0" => quirks.jump_v0 = true,
                "wrap" => quirks.wrap = true,
                "vblank" => quirks.vblank = true,
                _ => {
                    return Err(format!(
                        "unknown quirk {name:?}, expected octo, vip, schip, shift, load-store, vf-reset, jump-v0, wrap or vblank"
                    ))
                }
            }
        }
        Ok(quirks)
    }
}

/// Formats the quirks as the shortest list that parses back to them: the preset they
/// match, or the one needing the fewest quirks added, e.g. "octo,vf-reset".
impl std::fmt::Display for Quirks {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let flags = |q: &Quirks| {
            [
                (q.shift, "shift"),
                (q.load_store, "load-store"),
                (q.vf_reset, "vf-reset"),
                (q.jump_v0, "jump-v0"),
                (q.wrap, "wrap"),
                (q.vblank, "vblank"),
            ]
        };
        let best = ["octo", "vip", "schip"]
            .into_iter()
            .filter_map(|name| {
                let preset = flags(&Quirks::default().apply(name).ok()?);
                let ours = flags(self);
                // the preset's quirks must all be on
                if preset.iter().zip(&ours).any(|(p, o)| p.0 && !o.0) {
                    return None;
                }
                let added: Vec<&str> = preset
                    .iter()
                    .zip(&ours)
                    .filter(|(p, o)| o.0 && !p.0)
                    .map(|(_, o)| o.1)
                    .collect();
                Some((name, added))
            })
            .min_by_key(|(_, added)| added.len());
        let (name, added) = best.expect("schip has no quirks, so it always matches");
        write!(f, "{name}")?;
        for quirk in added {
            write!(f, ",{quirk}")?;
        }
        Ok(())
    }
}

/// Parses a quirk list (see `Quirks::apply`), starting from no quirks.
impl std::str::FromStr for Quirks {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Quirks::default().apply(s)
    }
}

pub struct Cpu {
    v: [u8; 16],
    i: u16,
    pc: u16,
    dt: u8,
    st: u8,
    memory: [u8; MEMORY_SIZE],
    /// Return addresses, holding at most `STACK_DEPTH` entries.
    stack: Vec<u16>,
    display: Display,
    keypad: Keypad,
    /// Register waiting for a key (FX0A); execution halts until it's set.
    wait_for_key: Option<usize>,
    /// The key pressed while waiting for one; FX0A stores it once it's released.
    key_down: Option<u8>,
    /// Set by 00FD; execution halts for good.
    exited: bool,
    /// Set by DXYN with the vblank quirk; execution halts until the next frame.
    wait_for_frame: bool,
    /// The SUPER-CHIP RPL user flags (FX75/FX85).
    flags: [u8; 16],
    quirks: Quirks,
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
        let big_font = BIG_FONT_START as usize;
        memory[big_font..big_font + BIG_FONT.len()].copy_from_slice(&BIG_FONT);
        Cpu {
            v: [0; 16],
            i: 0,
            pc: PROGRAM_START,
            dt: 0,
            st: 0,
            memory,
            stack: Vec::with_capacity(STACK_DEPTH),
            display: Display::new(),
            keypad: Keypad::default(),
            wait_for_key: None,
            key_down: None,
            exited: false,
            wait_for_frame: false,
            flags: [0; 16],
            quirks: Quirks::default(),
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

    pub fn set_quirks(&mut self, quirks: Quirks) {
        self.quirks = quirks;
    }

    /// Whether the program exited (00FD).
    pub fn exited(&self) -> bool {
        self.exited
    }

    pub fn display(&self) -> &Display {
        &self.display
    }

    pub fn display_mut(&mut self) -> &mut Display {
        &mut self.display
    }

    /// Starts a frame, decrementing the delay and sound timers; call at 60Hz. Returns
    /// true while the sound timer is active and a beep should be played.
    pub fn tick_timers(&mut self) -> bool {
        self.wait_for_frame = false;
        self.dt = self.dt.saturating_sub(1);
        if self.st > 0 {
            self.st -= 1;
            return true;
        }
        false
    }

    pub fn set_key(&mut self, key: u8, pressed: bool) {
        self.keypad.set(key, pressed);
        // like the COSMAC VIP, FX0A waits for a key to be pressed and then released
        if let Some(x) = self.wait_for_key {
            if pressed {
                self.key_down.get_or_insert(key);
            } else if self.key_down == Some(key) {
                self.v[x] = key;
                self.wait_for_key = None;
                self.key_down = None;
            }
        }
    }

    /// Fetches and executes a single opcode.
    pub fn step(&mut self) {
        // we're waiting for a key or the next frame, or the program is done, don't advance..
        if self.wait_for_key.is_some() || self.wait_for_frame || self.exited {
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
                0x00C0..=0x00CF => self.display.scroll_down(n as usize), // 00CN (scroll down N pixels)
                0x00E0 => self.display.clear(),                          // clear screen
                0x00EE => self.pc = self.stack.pop().unwrap_or(PROGRAM_START), // return from a subroutine
                0x00FB => self.display.scroll_right(4), // scroll right 4 pixels
                0x00FC => self.display.scroll_left(4),  // scroll left 4 pixels
                0x00FD => self.exited = true,           // exit the interpreter
                0x00FE => self.display.set_hires(false), // low resolution
                0x00FF => self.display.set_hires(true), // high resolution
                _ => {} // 0NNN (machine code routine) is unsupported
            },
            0x1000 => self.pc = nnn, // 1NNN (jump to NNN)
            0x2000 => {
                // 2NNN (call subroutine at NNN), the return address is dropped once the stack is full
                if self.stack.len() < STACK_DEPTH {
                    self.stack.push(self.pc);
                }
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
            // BXNN (PC = XNN + VX), or BNNN (PC = NNN + V0) with the jump-v0 quirk
            0xB000 => {
                let offset = if !self.quirks.jump_v0 {
                    self.v[x]
                } else {
                    self.v[0]
                };
                self.pc = (nnn + offset as u16) & 0xFFF;
            }
            0xC000 => self.v[x] = self.random() & nn, // CXNN (VX = random() & NN)
            0xD000 => self.draw(x, y, n),
            // only the lowest nibble of VX selects the key
            0xE000 => match nn {
                0x9E => self.skip_if(self.keypad.key(self.v[x] & 0xF)), // EX9E (skip if key VX is pressed)
                0xA1 => self.skip_if(!self.keypad.key(self.v[x] & 0xF)), // EXA1 (skip if key VX isn't pressed)
                _ => {}
            },
            0xF000 => self.xf000(x, nn),
            _ => unreachable!(),
        }
    }

    fn x8000(&mut self, x: usize, y: usize, n: u8) {
        let (vx, vy) = (self.v[x], self.v[y]);
        // the shift quirk shifts VY instead of VX
        let shifted = if self.quirks.shift { vy } else { vx };
        match n {
            0x0 => self.v[x] = vy, // 8XY0 (VX = VY)
            0x1..=0x3 => {
                match n {
                    0x1 => self.v[x] |= vy, // 8XY1 (VX = VX | VY)
                    0x2 => self.v[x] &= vy, // 8XY2 (VX = VX & VY)
                    _ => self.v[x] ^= vy,   // 8XY3 (VX = VX ^ VY)
                }
                if self.quirks.vf_reset {
                    self.v[0xF] = 0;
                }
            }
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
                self.v[x] = shifted >> 1;
                self.v[0xF] = shifted & 0x1;
            }
            0x7 => {
                // 8XY7 VX = VY - VX, with VF = NOT borrow
                self.v[x] = vy.wrapping_sub(vx);
                self.v[0xF] = (vy >= vx) as u8;
            }
            0xE => {
                // 8XYE VX << 1, VF = MSB
                self.v[x] = shifted << 1;
                self.v[0xF] = shifted >> 7;
            }
            _ => {}
        }
    }

    fn xf000(&mut self, x: usize, nn: u8) {
        let i = self.i as usize;
        match nn {
            0x07 => self.v[x] = self.dt,                            // FX07 (VX = DT)
            0x0A => self.wait_for_key = Some(x), // FX0A (wait for a key press and release, store it to VX)
            0x15 => self.dt = self.v[x],         // FX15 (DT = VX)
            0x18 => self.st = self.v[x],         // FX18 (ST = VX)
            0x1E => self.i = self.i.wrapping_add(self.v[x] as u16), // FX1E (I += VX)
            // FX29 (I = location of font for value of VX)
            0x29 => self.i = (self.v[x] & 0xF) as u16 * FONT_BYTE_LENGTH,
            // FX30 (I = location of big font for value of VX)
            0x30 => self.i = BIG_FONT_START + (self.v[x] & 0xF) as u16 * BIG_FONT_BYTE_LENGTH,
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
                self.advance_i(x);
            }
            0x65 => {
                // FX65 (V0..VX = I[0..X])
                for r in 0..=x {
                    self.v[r] = self.memory[(i + r) & 0xFFF];
                }
                self.advance_i(x);
            }
            0x75 => self.flags[..=x].copy_from_slice(&self.v[..=x]), // FX75 (flags[0..X] = V0..VX)
            0x85 => self.v[..=x].copy_from_slice(&self.flags[..=x]), // FX85 (V0..VX = flags[0..X])
            _ => {}
        }
    }

    /// With the load/store quirk, FX55/FX65 leave I past the last register.
    fn advance_i(&mut self, x: usize) {
        if self.quirks.load_store {
            self.i = self.i.wrapping_add(x as u16 + 1);
        }
    }

    /// DXYN: draw an 8xN sprite from memory at I to (VX, VY), VF = collision, or DXY0: a
    /// 16x16 sprite, two bytes per row.
    /// The start position wraps around the screen, and the sprite is clipped at its edges,
    /// or wraps too with the wrap quirk.
    fn draw(&mut self, x: usize, y: usize, n: u8) {
        let (width, height) = (self.display.width(), self.display.height());
        let vx = self.v[x] as usize % width;
        let vy = self.v[y] as usize % height;
        let (columns, rows) = if n == 0 { (16, 16) } else { (8, n as usize) };
        self.v[0xF] = 0;
        for yline in 0..rows {
            // the pixels of a row, leftmost in the highest bit
            let data = if columns == 16 {
                let at = self.i as usize + yline * 2;
                u16::from_be_bytes([self.memory[at & 0xFFF], self.memory[(at + 1) & 0xFFF]])
            } else {
                self.memory[(self.i as usize + yline) & 0xFFF] as u16
            };
            for xpix in 0..columns {
                if data & (1 << (columns - 1 - xpix)) != 0 {
                    let (mut px, mut py) = (vx + xpix, vy + yline);
                    if self.quirks.wrap {
                        px %= width;
                        py %= height;
                    }
                    if self.display.get(px, py) {
                        self.v[0xF] = 1;
                    }
                    self.display.flip(px, py);
                }
            }
        }
        self.wait_for_frame = self.quirks.vblank;
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
        run_with(Quirks::default(), program)
    }

    fn run_with(quirks: Quirks, program: &[u16]) -> Cpu {
        let rom: Vec<u8> = program.iter().flat_map(|op| op.to_be_bytes()).collect();
        let mut cpu = Cpu::with_seed(42);
        cpu.set_quirks(quirks);
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
    fn return_with_empty_stack_restarts_program() {
        let cpu = run(&[0x6001, 0x00EE]);
        assert_eq!(cpu.pc, PROGRAM_START);
    }

    #[test]
    fn call_depth_is_capped() {
        // 0x200: call 0x200, recursing forever
        let mut cpu = Cpu::with_seed(1);
        cpu.load(&[0x22, 0x00]).unwrap();
        for _ in 0..=STACK_DEPTH {
            cpu.step();
        }
        assert_eq!(cpu.stack.len(), STACK_DEPTH);
        assert!(cpu.stack.iter().all(|&addr| addr == 0x202));
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
    fn draw_wraps_start_position() {
        // the top row of font "0" at (66, 33) starts at (2, 1)
        let cpu = run(&[0x6042, 0x6121, 0xA000, 0xD011]);
        assert!(cpu.display.get(2, 1));
        assert!(cpu.display.get(5, 1));
        assert!(!cpu.display.get(6, 1));
    }

    #[test]
    fn draw_with_vf_as_coordinate() {
        // VF = 10 is read as the X coordinate before it becomes the collision flag
        let cpu = run(&[0x6F0A, 0x6100, 0xA000, 0xDF11]);
        assert!(cpu.display.get(10, 0));
        assert!(!cpu.display.get(0, 0));
        assert_eq!(cpu.v[0xF], 0);

        // with a pixel already lit at (10, 0), the collision is found and the rest of
        // the row is still drawn at x = 11..13. 0x8000 at 0x210 is the one-pixel
        // sprite; it and the 0x0000 before it also run as no-ops at the end.
        let cpu = run(&[
            0x620A, 0x6100, 0xA210, 0xD211, 0x6F0A, 0xA000, 0xDF11, 0x0000, 0x8000,
        ]);
        assert!(!cpu.display.get(10, 0));
        assert!(cpu.display.get(11, 0));
        assert!(cpu.display.get(13, 0));
        assert!(!cpu.display.get(0, 0));
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn wait_for_key() {
        let mut cpu = run(&[0xF30A, 0x6001]);
        assert_eq!(cpu.pc, 0x202); // halted on the instruction after FX0A
        assert_eq!(cpu.v[0], 0);
        cpu.set_key(0xB, false); // a release without a press doesn't resume
        cpu.set_key(0xB, true);
        cpu.set_key(0xC, true); // the first key pressed is the one waited for
        cpu.set_key(0xC, false);
        cpu.step();
        assert_eq!(cpu.v[0], 0); // still waiting for the key to be released
        cpu.set_key(0xB, false);
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

        // only the lowest nibble of VX selects the key, so 0x15 is key 5
        let mut cpu = Cpu::with_seed(1);
        cpu.load(&[0x60, 0x15, 0xE0, 0x9E, 0x00, 0x00, 0xE0, 0xA1])
            .unwrap();
        cpu.set_key(5, true);
        cpu.step();
        cpu.step();
        assert_eq!(cpu.pc, 0x206);
        cpu.step();
        assert_eq!(cpu.pc, 0x208); // EXA1 doesn't skip either
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

    #[test]
    fn parse_quirks() {
        let quirks: Quirks = "octo".parse().unwrap();
        assert_eq!(
            quirks,
            Quirks {
                shift: true,
                load_store: true,
                wrap: true,
                jump_v0: true,
                ..Quirks::default()
            }
        );

        // presets reset the quirks named before them, single quirks add to them
        let quirks: Quirks = "wrap,vip,vf-reset".parse().unwrap();
        assert_eq!(
            quirks,
            Quirks {
                shift: true,
                load_store: true,
                vf_reset: true,
                jump_v0: true,
                wrap: false,
                vblank: true,
            }
        );

        let quirks: Quirks = "octo,schip,jump-v0".parse().unwrap();
        assert_eq!(
            quirks,
            Quirks {
                jump_v0: true,
                ..Quirks::default()
            }
        );

        assert!("schip,bogus"
            .parse::<Quirks>()
            .unwrap_err()
            .contains("\"bogus\""));
    }

    #[test]
    fn display_quirks() {
        for list in [
            "octo",
            "vip",
            "schip",
            "octo,vf-reset",
            "schip,wrap",
            "schip,shift,load-store,jump-v0",
            "octo,vblank",
            "vip,wrap",
        ] {
            assert_eq!(list.parse::<Quirks>().unwrap().to_string(), list);
        }
        // the same quirks as vip,wrap, which needs fewer added
        let quirks: Quirks = "octo,vf-reset,vblank".parse().unwrap();
        assert_eq!(quirks.to_string(), "vip,wrap");
    }

    #[test]
    fn shift_quirk_shifts_vy() {
        let quirks = Quirks {
            shift: true,
            ..Quirks::default()
        };
        let cpu = run_with(quirks, &[0x6081, 0x6103, 0x8016]);
        assert_eq!(cpu.v[0], 0x01);
        assert_eq!(cpu.v[0xF], 1);

        let cpu = run_with(quirks, &[0x6001, 0x6181, 0x801E]);
        assert_eq!(cpu.v[0], 0x02);
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn load_store_quirk_advances_i() {
        let quirks = Quirks {
            load_store: true,
            ..Quirks::default()
        };
        assert_eq!(run_with(quirks, &[0xA300, 0xF255]).i, 0x303);
        assert_eq!(run_with(quirks, &[0xA300, 0xF065]).i, 0x301);
    }

    #[test]
    fn vf_reset_quirk() {
        assert_eq!(run(&[0x6F05, 0x8011]).v[0xF], 5);

        let quirks = Quirks {
            vf_reset: true,
            ..Quirks::default()
        };
        for op in [0x8011, 0x8012, 0x8013] {
            assert_eq!(run_with(quirks, &[0x6F05, op]).v[0xF], 0);
        }
    }

    #[test]
    fn vblank_quirk_waits_for_the_next_frame() {
        assert_eq!(run(&[0xA000, 0xD001, 0x6001]).v[0], 1);

        let quirks = Quirks {
            vblank: true,
            ..Quirks::default()
        };
        let mut cpu = run_with(quirks, &[0xA000, 0xD001, 0x6001]);
        assert_eq!(cpu.v[0], 0);
        assert_eq!(cpu.pc, 0x204);
        cpu.tick_timers();
        cpu.step();
        assert_eq!(cpu.v[0], 1);
    }

    #[test]
    fn jump_v0_quirk_uses_v0() {
        // B300 jumps to 0x300 + V3 like SUPER-CHIP, or to 0x300 + V0 with the quirk
        assert_eq!(run(&[0x6004, 0x6302, 0xB300]).pc, 0x302);

        let quirks = Quirks {
            jump_v0: true,
            ..Quirks::default()
        };
        assert_eq!(run_with(quirks, &[0x6004, 0x6302, 0xB300]).pc, 0x304);
    }

    #[test]
    fn wrap_quirk_wraps_sprites() {
        // the top row of font "0" at (62, 31) is clipped after two pixels by default
        let cpu = run(&[0x603E, 0x611F, 0xA000, 0xD011]);
        assert!(cpu.display.get(63, 31));
        assert!(!cpu.display.get(0, 31));

        let quirks = Quirks {
            wrap: true,
            ..Quirks::default()
        };
        let cpu = run_with(quirks, &[0x603E, 0x611F, 0xA000, 0xD012]);
        assert!(cpu.display.get(63, 31));
        assert!(cpu.display.get(0, 31));
        assert!(cpu.display.get(1, 31));
        assert!(!cpu.display.get(2, 31));
        // the second row wraps to the top
        assert!(cpu.display.get(62, 0));
        assert!(cpu.display.get(1, 0));
    }

    #[test]
    fn high_resolution() {
        let cpu = run(&[0x00FF]);
        assert!(cpu.display.hires());
        assert_eq!((cpu.display.width(), cpu.display.height()), (128, 64));

        // font "0" at (100, 50) is only on screen in high resolution
        let cpu = run(&[0x00FF, 0x6064, 0x6132, 0xA000, 0xD015]);
        assert!(cpu.display.get(100, 50));
        assert!(cpu.display.pixel(100, 50));
        assert!(!cpu.display.pixel(101, 51));

        // switching back clears the screen, and low resolution pixels cover 2x2 frame pixels
        let cpu = run(&[0x00FF, 0xA000, 0xD015, 0x00FE, 0xD011]);
        assert!(!cpu.display.hires());
        assert_eq!(cpu.display.width(), 64);
        assert!(cpu.display.get(3, 0));
        assert!(cpu.display.pixel(7, 1));
        assert!(!cpu.display.get(0, 1));
    }

    #[test]
    fn draw_large_sprite() {
        // a 16x16 sprite of 0xFF00 rows from 0x300, at (120, 60) in high resolution,
        // clipped at the edges, then drawn again
        let program = [0x00FF, 0x6078, 0x613C, 0xA300, 0xD010, 0xD010];
        let mut rom: Vec<u8> = program
            .iter()
            .flat_map(|op: &u16| op.to_be_bytes())
            .collect();
        rom.resize(0x100, 0);
        for _ in 0..16 {
            rom.extend([0xFF, 0x00]);
        }
        let mut cpu = Cpu::with_seed(1);
        cpu.load(&rom).unwrap();
        for _ in 0..5 {
            cpu.step();
        }
        assert!(cpu.display.get(120, 60));
        assert!(cpu.display.get(127, 63));
        assert!(!cpu.display.get(0, 60));
        assert!(!cpu.display.get(120, 0));
        assert_eq!(cpu.v[0xF], 0);

        cpu.step();
        assert!(!cpu.display.get(120, 60));
        assert_eq!(cpu.v[0xF], 1);
    }

    #[test]
    fn scroll() {
        // the top row of font "0" (4 pixels) at (8, 0)
        let cpu = run(&[0x6008, 0x6100, 0xA000, 0xD011, 0x00C3]);
        assert!(!cpu.display.get(8, 0));
        assert!(cpu.display.get(8, 3));

        let cpu = run(&[0x6008, 0x6100, 0xA000, 0xD011, 0x00FB]);
        assert!(!cpu.display.get(8, 0));
        assert!(cpu.display.get(12, 0));
        assert!(cpu.display.get(15, 0));

        let cpu = run(&[0x6008, 0x6100, 0xA000, 0xD011, 0x00FC]);
        assert!(cpu.display.get(4, 0));
        assert!(!cpu.display.get(8, 0));

        // pixels scrolled off the screen are gone
        let cpu = run(&[0x6000, 0x6100, 0xA000, 0xD011, 0x00FC, 0x00FB]);
        assert!(!cpu.display.get(0, 0));
        assert!(!cpu.display.get(4, 0));

        // in high resolution, scrolling is by high resolution pixels
        let cpu = run(&[0x00FF, 0x6008, 0x6100, 0xA000, 0xD011, 0x00FB, 0x00C1]);
        assert!(cpu.display.get(12, 1));
        assert!(!cpu.display.get(11, 1));
    }

    #[test]
    fn big_font() {
        let cpu = run(&[0x6007, 0xF030]);
        assert_eq!(cpu.i, BIG_FONT_START + 7 * BIG_FONT_BYTE_LENGTH);
        assert_eq!(cpu.memory[cpu.i as usize], 0xFF); // the top of the 7
    }

    #[test]
    fn save_and_restore_flags() {
        let cpu = run(&[
            0x6001, 0x6102, 0x6203, 0xF275, 0x6000, 0x6100, 0x6200, 0xF185,
        ]);
        assert_eq!(&cpu.v[..3], &[1, 2, 0]);
    }

    #[test]
    fn exit() {
        let cpu = run(&[0x00FD, 0x6001]);
        assert!(cpu.exited());
        assert_eq!(cpu.pc, 0x202);
        assert_eq!(cpu.v[0], 0);
    }
}
