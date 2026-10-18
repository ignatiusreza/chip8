//! The command line options, with the ROM and its settings file.

use std::path::Path;
use std::str::FromStr;

use chip8::{settings, Quirks};

use super::Result;

const USAGE: &str = "Usage : {} [--display tui|window] [--quirks LIST] [--speed N] ROMNAME

  --display tui     draw in this terminal (default)
  --display window  open a separate window
  --quirks LIST     comma separated presets (octo, vip, schip) and quirks
                    (shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on,
                    starting from the default
  --speed N         opcodes run per frame

Without --quirks or --speed, they are read from the ROM's .cfg file if it has one,
otherwise they default to {quirks} and {speed}.";

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

/// What to run and how: the command line, with the ROM and its settings file.
pub struct Options {
    pub rom_path: String,
    pub rom: Vec<u8>,
    pub mode: DisplayMode,
    /// The quirks the ROM runs with: from the command line, otherwise its settings file,
    /// otherwise the defaults.
    pub quirks: Quirks,
    /// The opcodes run per frame, chosen like the quirks.
    pub speed: u32,
    /// The ROM's settings file, with those quirks and speed, for the terminal to show.
    pub settings: settings::RomSettings,
}

impl Options {
    /// Parses the command line (`args` starting with the program name), then reads the ROM
    /// and its settings file. Returns `None` if there is nothing to run, after printing the
    /// usage.
    pub fn parse(mut args: impl Iterator<Item = String>) -> Result<Option<Options>> {
        let program = args.next().unwrap_or_else(|| "chip8".into());
        let usage = USAGE
            .replace("{}", &program)
            .replace("{quirks}", settings::DEFAULT_QUIRKS)
            .replace("{speed}", &settings::DEFAULT_SPEED.to_string());

        let mut mode = DisplayMode::default();
        // set from the command line, otherwise from the ROM's settings file
        let (mut quirks, mut speed) = (None, None);
        let mut rom_path = None;
        while let Some(arg) = args.next() {
            if let Some(value) = option(&arg, "--display", &mut args)? {
                mode = value.parse()?;
            } else if let Some(value) = option(&arg, "--quirks", &mut args)? {
                quirks = Some(settings::parse_quirks(&value)?);
            } else if let Some(value) = option(&arg, "--speed", &mut args)? {
                speed = Some(settings::parse_speed(&value).map_err(|e| format!("--{e}"))?);
            } else if arg == "-h" || arg == "--help" {
                println!("{usage}");
                return Ok(None);
            } else if rom_path.is_none() {
                rom_path = Some(arg);
            } else {
                return Err(format!("unexpected argument {arg:?}\n\n{usage}").into());
            }
        }
        let Some(rom_path) = rom_path else {
            println!("{usage}");
            return Ok(None);
        };

        let rom =
            std::fs::read(&rom_path).map_err(|e| format!("Could not read {rom_path}: {e}"))?;
        let settings = settings::load(Path::new(&rom_path))?;
        let quirks = quirks
            .or(settings.quirks)
            .unwrap_or_else(settings::default_quirks);
        let speed = speed.or(settings.speed).unwrap_or(settings::DEFAULT_SPEED);
        Ok(Some(Options {
            rom_path,
            rom,
            mode,
            quirks,
            speed,
            settings: settings::RomSettings {
                quirks: Some(quirks),
                speed: Some(speed),
                ..settings
            },
        }))
    }
}

/// The value of option `name` if `arg` is it, given as "--name=VALUE" or "--name VALUE"
/// (taking VALUE from `args`).
fn option(
    arg: &str,
    name: &str,
    args: &mut impl Iterator<Item = String>,
) -> Result<Option<String>> {
    if let Some(value) = arg
        .strip_prefix(name)
        .and_then(|rest| rest.strip_prefix('='))
    {
        return Ok(Some(value.to_string()));
    }
    if arg != name {
        return Ok(None);
    }
    Ok(Some(args.next().ok_or(format!("{name} needs a value"))?))
}
