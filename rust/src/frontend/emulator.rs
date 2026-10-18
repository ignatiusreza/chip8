use std::path::Path;

use chip8::{rom, Cpu, Rom, RomSettings};

use super::screen::Screen;
use super::sound::Sound;
use super::terminal::Terminal;
use super::window::Window;
use super::{Args, DisplayMode, Result};

/// A CPU running a ROM, with its display, keypad and beeper hooked up.
pub struct Emulator {
    cpu: Cpu,
    sound: Sound,
    screen: Box<dyn Screen>,
    /// Opcodes run per frame.
    speed: u32,
    /// Set once Esc was pressed, or the window closed.
    quit: bool,
}

impl Emulator {
    /// Loads the ROM and its settings file, then opens the audio device and the screen.
    pub fn new(args: &Args) -> Result<Self> {
        let rom_file = Rom::load(Path::new(&args.rom_path))?;
        // the command line wins over the ROM's settings file, which wins over the defaults
        let quirks = args
            .quirks
            .or(rom_file.settings.quirks)
            .unwrap_or_else(rom::default_quirks);
        let speed = args
            .speed
            .or(rom_file.settings.speed)
            .unwrap_or(rom::DEFAULT_SPEED);

        let mut cpu = Cpu::new();
        cpu.set_quirks(quirks);
        cpu.load(&rom_file.data)?;
        // open the audio device first, as it may log to the terminal
        let sound = Sound::new();
        let title = format!("Chip 8 : {}", args.rom_path);
        let screen: Box<dyn Screen> = match args.mode {
            // the terminal shows the quirks and speed the ROM runs with, alongside its how to play
            DisplayMode::Terminal => Box::new(Terminal::new(
                &title,
                &RomSettings {
                    quirks: Some(quirks),
                    speed: Some(speed),
                    ..rom_file.settings
                },
            )?),
            DisplayMode::Window => Box::new(Window::new(&title)?),
        };
        Ok(Emulator {
            cpu,
            sound,
            screen,
            speed,
            quit: false,
        })
    }

    /// Runs the ROM until Esc is pressed or the window is closed.
    pub fn run(&mut self) -> Result<()> {
        while !self.quit {
            self.tick()?;
        }
        Ok(())
    }

    /// Runs one 1/60s frame: timers, keyboard, a few opcodes, then the screen.
    fn tick(&mut self) -> Result<()> {
        self.sound.set_playing(self.cpu.tick_timers());
        let keys = self.screen.keys();
        for (k, &down) in keys.down.iter().enumerate() {
            self.cpu.set_key(k as u8, down);
        }
        self.quit = keys.quit;

        for _ in 0..self.speed {
            self.cpu.step();
        }

        // update screen if invalidated
        self.screen.draw(self.cpu.display_mut())?;
        Ok(())
    }
}
