//! The command line arguments.

use std::str::FromStr;

use chip8::{rom, Quirks};

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

/// The command line: the ROM to run and how.
pub struct Args {
    pub rom_path: String,
    pub mode: DisplayMode,
    /// The quirks and speed to run the ROM with, if given, instead of the ones from its
    /// settings file.
    pub quirks: Option<Quirks>,
    pub speed: Option<u32>,
}

impl Args {
    /// Parses the command line, `args` starting with the program name. Returns `None` if
    /// there is nothing to run, after printing the usage.
    pub fn parse(mut args: impl Iterator<Item = String>) -> Result<Option<Args>> {
        let program = args.next().unwrap_or_else(|| "chip8".into());
        let usage = USAGE
            .replace("{}", &program)
            .replace("{quirks}", rom::DEFAULT_QUIRKS)
            .replace("{speed}", &rom::DEFAULT_SPEED.to_string());

        let mut mode = DisplayMode::default();
        let (mut quirks, mut speed) = (None, None);
        let mut rom_path = None;
        while let Some(arg) = args.next() {
            if let Some(value) = option(&arg, "--display", &mut args)? {
                mode = value.parse()?;
            } else if let Some(value) = option(&arg, "--quirks", &mut args)? {
                quirks = Some(rom::parse_quirks(&value)?);
            } else if let Some(value) = option(&arg, "--speed", &mut args)? {
                speed = Some(rom::parse_speed(&value).map_err(|e| format!("--{e}"))?);
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

        Ok(Some(Args {
            rom_path,
            mode,
            quirks,
            speed,
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
