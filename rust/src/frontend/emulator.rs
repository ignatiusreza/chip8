use std::path::Path;

use chip8::{settings, Cpu};

use super::input::Input;
use super::screen::Screen;
use super::sound::Sound;
use super::terminal::Terminal;
use super::window::Window;
use super::{Args, DisplayMode, Result};

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
    /// Loads the ROM and its settings file, then opens the audio device and the screen.
    pub fn new(args: &Args) -> Result<Self> {
        let rom_path = &args.rom_path;
        let rom = std::fs::read(rom_path).map_err(|e| format!("Could not read {rom_path}: {e}"))?;
        // the command line wins over the ROM's settings file, which wins over the defaults
        let settings = settings::load(Path::new(rom_path))?;
        let quirks = args
            .quirks
            .or(settings.quirks)
            .unwrap_or_else(settings::default_quirks);
        let speed = args
            .speed
            .or(settings.speed)
            .unwrap_or(settings::DEFAULT_SPEED);

        let mut cpu = Cpu::new();
        cpu.set_quirks(quirks);
        cpu.load(&rom)?;
        // open the audio device first, as it may log to the terminal
        let sound = Sound::new();
        let title = format!("Chip 8 : {rom_path}");
        let screen: Box<dyn Screen> = match args.mode {
            // the terminal shows the quirks and speed the ROM runs with, alongside its how to play
            DisplayMode::Terminal => Box::new(Terminal::new(
                &title,
                &settings::RomSettings {
                    quirks: Some(quirks),
                    speed: Some(speed),
                    ..settings
                },
            )?),
            DisplayMode::Window => Box::new(Window::new(&title)?),
        };
        Ok(Emulator {
            cpu,
            input: Input::default(),
            sound,
            screen,
            speed,
        })
    }

    /// Runs the ROM until Esc is pressed or the window is closed.
    pub fn run(&mut self) -> Result<()> {
        while !self.input.quit_requested() {
            self.tick()?;
        }
        Ok(())
    }

    /// Runs one 1/60s frame: timers, keyboard, a few opcodes, then the screen.
    fn tick(&mut self) -> Result<()> {
        self.sound.set_playing(self.cpu.tick_timers());
        self.input.update(&self.screen.keys(), &mut self.cpu);

        for _ in 0..self.speed {
            self.cpu.step();
        }

        // update screen if invalidated
        self.screen.draw(self.cpu.display_mut())?;
        Ok(())
    }
}
