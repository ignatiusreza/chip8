mod frontend;

use frontend::{DisplayMode, Emulator};

const USAGE: &str = "Usage : {} [--display tui|window] ROMNAME

  --display tui     draw in this terminal (default)
  --display window  open a separate window";

fn main() -> frontend::Result<()> {
    let mut args = std::env::args();
    let program = args.next().unwrap_or_else(|| "chip8".into());
    let usage = USAGE.replace("{}", &program);

    let mut mode = DisplayMode::default();
    let mut rom_path = None;
    while let Some(arg) = args.next() {
        if let Some(value) = arg.strip_prefix("--display=") {
            mode = value.parse()?;
        } else if arg == "--display" {
            mode = args.next().ok_or("--display needs a value")?.parse()?;
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
    let mut emulator = Emulator::new(&format!("Chip 8 : {rom_path}"), &rom, mode)?;

    while emulator.is_running() {
        emulator.tick()?;
    }

    Ok(())
}
