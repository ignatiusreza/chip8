mod frontend;

use chip8::settings;
use frontend::{DisplayMode, Emulator};

const USAGE: &str = "Usage : {} [--display tui|window] [--quirks LIST] [--speed N] ROMNAME

  --display tui     draw in this terminal (default)
  --display window  open a separate window
  --quirks LIST     comma separated presets (octo, vip, schip) and quirks
                    (shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on,
                    starting from the default
  --speed N         opcodes run per frame

Without --quirks or --speed, they are read from the ROM's .cfg file if it has one,
otherwise they default to {quirks} and {speed}.";

fn main() -> frontend::Result<()> {
    let mut args = std::env::args();
    let program = args.next().unwrap_or_else(|| "chip8".into());
    let usage = USAGE
        .replace("{}", &program)
        .replace("{quirks}", settings::DEFAULT_QUIRKS)
        .replace("{speed}", &settings::DEFAULT_SPEED.to_string());

    let mut mode = DisplayMode::default();
    // set from the command line, otherwise from the ROM's settings file
    let mut options = settings::RomSettings::default();
    let mut rom_path = None;
    while let Some(arg) = args.next() {
        if let Some(value) = arg.strip_prefix("--display=") {
            mode = value.parse()?;
        } else if arg == "--display" {
            mode = args.next().ok_or("--display needs a value")?.parse()?;
        } else if let Some(value) = arg.strip_prefix("--quirks=") {
            options.quirks = Some(settings::parse_quirks(value)?);
        } else if arg == "--quirks" {
            let value = args.next().ok_or("--quirks needs a value")?;
            options.quirks = Some(settings::parse_quirks(&value)?);
        } else if let Some(value) = arg.strip_prefix("--speed=") {
            options.speed = Some(settings::parse_speed(value).map_err(|e| format!("--{e}"))?);
        } else if arg == "--speed" {
            let value = args.next().ok_or("--speed needs a value")?;
            options.speed = Some(settings::parse_speed(&value).map_err(|e| format!("--{e}"))?);
        } else if arg == "-h" || arg == "--help" {
            println!("{usage}");
            return Ok(());
        } else if rom_path.is_none() {
            rom_path = Some(arg);
        } else {
            return Err(format!("unexpected argument {arg:?}\n\n{usage}").into());
        }
    }
    let Some(rom_path) = rom_path else {
        println!("{usage}");
        return Ok(());
    };

    let rom = std::fs::read(&rom_path).map_err(|e| format!("Could not read {rom_path}: {e}"))?;
    let rom_settings = settings::load(std::path::Path::new(&rom_path))?;
    let quirks = options
        .quirks
        .or(rom_settings.quirks)
        .unwrap_or_else(settings::default_quirks);
    let speed = options
        .speed
        .or(rom_settings.speed)
        .unwrap_or(settings::DEFAULT_SPEED);
    let mut emulator = Emulator::new(
        &format!("Chip 8 : {rom_path}"),
        &rom,
        mode,
        quirks,
        speed,
        // the terminal shows the quirks and speed the ROM runs with, alongside its how to play
        &settings::RomSettings {
            quirks: Some(quirks),
            speed: Some(speed),
            ..rom_settings
        },
    )?;

    while emulator.is_running() {
        emulator.tick()?;
    }

    Ok(())
}
