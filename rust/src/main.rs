mod frontend;

use frontend::Emulator;

fn main() -> frontend::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        println!("Usage : {} ROMNAME", args[0]);
        return Ok(());
    }

    let rom = std::fs::read(&args[1]).map_err(|e| format!("Could not read {}: {e}", args[1]))?;
    let mut emulator = Emulator::new(&format!("Chip 8 : {}", args[1]), &rom)?;

    while emulator.is_running() {
        emulator.tick()?;
    }

    Ok(())
}
