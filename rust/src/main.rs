mod frontend;

use frontend::{Args, Emulator};

fn main() -> frontend::Result<()> {
    let Some(args) = Args::parse(std::env::args())? else {
        return Ok(()); // printed the usage
    };
    Emulator::new(&args)?.run()
}
