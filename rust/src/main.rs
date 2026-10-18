mod frontend;

use frontend::{Emulator, Options};

fn main() -> frontend::Result<()> {
    let Some(options) = Options::parse(std::env::args())? else {
        return Ok(()); // printed the usage
    };
    let mut emulator = Emulator::new(&options)?;

    while emulator.is_running() {
        emulator.tick()?;
    }

    Ok(())
}
