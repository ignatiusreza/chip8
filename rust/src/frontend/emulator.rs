use chip8::Cpu;

use super::input::Input;
use super::screen::Screen;
use super::sound::Sound;
use super::terminal::Terminal;
use super::window::Window;
use super::{DisplayMode, Options, Result};

/// A CPU running a ROM, with its display, keypad and beeper hooked up.
pub struct Emulator {
    cpu: Cpu,
    input: Input,
    sound: Sound,
    screen: Box<dyn Screen>,
    /// Opcodes run per frame.
    speed: u32,
}

impl Emulator {
    /// Loads the ROM, then opens the audio device and the screen.
    pub fn new(options: &Options) -> Result<Self> {
        let mut cpu = Cpu::new();
        cpu.set_quirks(options.quirks);
        cpu.load(&options.rom)?;
        // open the audio device first, as it may log to the terminal
        let sound = Sound::new();
        let title = format!("Chip 8 : {}", options.rom_path);
        let screen: Box<dyn Screen> = match options.mode {
            DisplayMode::Terminal => Box::new(Terminal::new(&title, &options.settings)?),
            DisplayMode::Window => Box::new(Window::new(&title)?),
        };
        Ok(Emulator {
            cpu,
            input: Input::default(),
            sound,
            screen,
            speed: options.speed,
        })
    }

    /// Runs one 1/60s frame: timers, keyboard, a few opcodes, then the screen.
    pub fn tick(&mut self) -> Result<()> {
        self.sound.set_playing(self.cpu.tick_timers());
        self.input.update(&self.screen.keys(), &mut self.cpu);

        for _ in 0..self.speed {
            self.cpu.step();
        }

        // update screen if invalidated
        self.screen.draw(self.cpu.display_mut())?;
        Ok(())
    }

    pub fn is_running(&self) -> bool {
        !self.input.quit_requested()
    }
}
