mod frontend;

use frontend::{Emulator, Options};

fn main() -> frontend::Result<()> {
    let Some(options) = Options::parse(std::env::args())? else {
        return Ok(()); // printed the usage
    };
    Emulator::new(&options)?.run()
}
