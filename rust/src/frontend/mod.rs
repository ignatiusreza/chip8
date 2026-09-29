//! Wires the backend-free core to a screen (a terminal or a window), keyboard and
//! speaker.

mod input;
mod sound;
mod terminal;
mod window;

use std::str::FromStr;

use chip8::{Cpu, Display};
use input::{Input, Keys};
use sound::Sound;
use terminal::Terminal;
use window::Window;

pub type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;

const OPCODES_PER_TICK: usize = 3;

/// Where the display is shown and the keyboard read from.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum DisplayMode {
    /// The terminal the emulator was started from.
    #[default]
    Terminal,
    /// A separate desktop window.
    Window,
}

impl FromStr for DisplayMode {
    type Err = String;

    fn from_str(s: &str) -> std::result::Result<Self, Self::Err> {
        match s {
            "tui" | "terminal" => Ok(DisplayMode::Terminal),
            "window" | "gui" => Ok(DisplayMode::Window),
            _ => Err(format!("unknown display {s:?}, expected tui or window")),
        }
    }
}

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
}

impl Emulator {
    /// Loads the ROM, then opens the audio device and the screen.
    pub fn new(title: &str, rom: &[u8], mode: DisplayMode) -> Result<Self> {
        let mut cpu = Cpu::new();
        cpu.load(rom)?;
        // open the audio device first, as it may log to the terminal
        let sound = Sound::new();
        let screen: Box<dyn Screen> = match mode {
            DisplayMode::Terminal => Box::new(Terminal::new(title)?),
            DisplayMode::Window => Box::new(Window::new(title)?),
        };
        Ok(Emulator {
            cpu,
            input: Input::default(),
            sound,
            screen,
        })
    }

    /// Runs one 1/60s frame: timers, keyboard, a few opcodes, then the screen.
    pub fn tick(&mut self) -> Result<()> {
        self.sound.set_playing(self.cpu.tick_timers());
        self.input.update(&self.screen.keys(), &mut self.cpu);

        for _ in 0..OPCODES_PER_TICK {
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
