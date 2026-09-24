//! Wires the backend-free core to a window, keyboard and speaker.

mod graphic;
mod input;
mod sound;

use chip8::Cpu;
use graphic::Graphic;
use input::Input;
use sound::Sound;

pub type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;

const OPCODES_PER_TICK: usize = 3;

/// A CPU running a ROM, with its display, keypad and beeper hooked up.
pub struct Emulator {
    cpu: Cpu,
    graphic: Graphic,
    input: Input,
    sound: Sound,
}

impl Emulator {
    /// Loads the ROM, then opens the window and audio device.
    pub fn new(title: &str, rom: &[u8]) -> Result<Self> {
        let mut cpu = Cpu::new();
        cpu.load(rom)?;
        Ok(Emulator {
            cpu,
            graphic: Graphic::new(title)?,
            input: Input::default(),
            sound: Sound::new(),
        })
    }

    /// Runs one 1/60s frame: timers, keyboard, a few opcodes, then the screen.
    pub fn tick(&mut self) -> Result<()> {
        self.sound.set_playing(self.cpu.tick_timers());
        self.input.update(self.graphic.window(), &mut self.cpu);

        for _ in 0..OPCODES_PER_TICK {
            self.cpu.step();
        }

        // update screen if invalidated
        self.graphic.update(self.cpu.display_mut())?;
        Ok(())
    }

    pub fn is_running(&self) -> bool {
        self.graphic.is_open() && !self.input.quit_requested()
    }
}
