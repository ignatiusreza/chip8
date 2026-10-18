//! Wires the backend-free core to a screen (a terminal or a window), keyboard and
//! speaker.

mod input;
mod layout;
mod options;
mod sound;
mod terminal;
mod window;

use chip8::{Cpu, Display};
use input::{Input, Keys};
pub use options::{DisplayMode, Options};
use sound::Sound;
use terminal::Terminal;
use window::Window;

pub type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;

/// A frontend that shows the display and reads the keyboard, once per frame.
trait Screen {
    /// The keys held down right now.
    fn keys(&mut self) -> Keys;
    /// Shows the display, then waits for the next 1/60s frame.
    fn draw(&mut self, display: &mut Display) -> Result<()>;
}

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
