use std::num::NonZero;

use chip8::{Cpu, Display};
use minifb::{Key, Scale, Window, WindowOptions};
use rodio::source::{Function, SignalGenerator, Source};
use rodio::{DeviceSinkBuilder, Player};

const FPS: usize = 60;
const OPCODES_PER_TICK: usize = 3;
const BEEP_HZ: f32 = 440.0;
const BLACK: u32 = 0x000000;
const WHITE: u32 = 0xFFFFFF;

// CHIP-8 keypad value for each index, mapped onto the left side of a QWERTY keyboard
const KEYMAP: [Key; 16] = [
    Key::Key1, // 0
    Key::Q,    // 1
    Key::W,    // 2
    Key::E,    // 3
    Key::A,    // 4
    Key::S,    // 5
    Key::D,    // 6
    Key::Z,    // 7
    Key::X,    // 8
    Key::C,    // 9
    Key::R,    // A
    Key::F,    // B
    Key::V,    // C
    Key::T,    // D
    Key::G,    // E
    Key::B,    // F
];

/// Square-wave beeper that plays while the sound timer is active.
struct Beeper {
    // the device sink must stay alive for the player to keep producing sound
    _sink: rodio::MixerDeviceSink,
    player: Player,
}

impl Beeper {
    fn new() -> Result<Self, Box<dyn std::error::Error>> {
        let sink = DeviceSinkBuilder::open_default_sink()?;
        let player = Player::connect_new(sink.mixer());
        let sample_rate = NonZero::new(44100).unwrap();
        player.append(SignalGenerator::new(sample_rate, BEEP_HZ, Function::Square).amplify(0.1));
        player.pause();
        Ok(Beeper {
            _sink: sink,
            player,
        })
    }

    fn set_playing(&self, playing: bool) {
        if playing {
            self.player.play();
        } else {
            self.player.pause();
        }
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        println!("Usage : {} ROMNAME", args[0]);
        return Ok(());
    }

    let rom = std::fs::read(&args[1]).map_err(|e| format!("Could not read {}: {e}", args[1]))?;
    let mut cpu = Cpu::new();
    cpu.load(&rom)?;

    let mut window = Window::new(
        &format!("Chip 8 : {}", args[1]),
        Display::WIDTH,
        Display::HEIGHT,
        WindowOptions {
            scale: Scale::X8,
            ..WindowOptions::default()
        },
    )?;
    window.set_target_fps(FPS);

    // keep running without sound when no audio device is available
    let beeper = Beeper::new()
        .inspect_err(|e| eprintln!("Sound disabled, could not open audio device: {e}"))
        .ok();

    let mut buffer = vec![BLACK; Display::BUFF_LENGTH];
    let mut keys = [false; 16];

    while window.is_open() && !window.is_key_down(Key::Escape) {
        let beeping = cpu.tick_timers();
        if let Some(beeper) = &beeper {
            beeper.set_playing(beeping);
        }

        // only report changes, so FX0A waits for a fresh keypress
        for (k, key) in KEYMAP.iter().enumerate() {
            let down = window.is_key_down(*key);
            if down != keys[k] {
                keys[k] = down;
                cpu.set_key(k as u8, down);
            }
        }

        for _ in 0..OPCODES_PER_TICK {
            cpu.step();
        }

        // redraw only if invalidated, but always update so input keeps being polled
        if cpu.display_mut().take_invalidated() {
            let display = cpu.display();
            for y in 0..Display::HEIGHT {
                for x in 0..Display::WIDTH {
                    buffer[x + y * Display::WIDTH] = if display.get(x, y) { WHITE } else { BLACK };
                }
            }
            window.update_with_buffer(&buffer, Display::WIDTH, Display::HEIGHT)?;
        } else {
            window.update();
        }
    }

    Ok(())
}
