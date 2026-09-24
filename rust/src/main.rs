use std::time::Duration;

use chip8::{Cpu, Display};
use sdl2::audio::{AudioCallback, AudioSpecDesired};
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::pixels::Color;
use sdl2::rect::Rect;

const SCALE: u32 = 10;
const FREQUENCY: i32 = 44100;
const BEEP_MS: i32 = 15;
const OPCODES_PER_TICK: usize = 3;

/// Square-wave beeper; `beep` queues a tone for the given number of ms.
struct Beeper {
    samples_left: i32,
    phase: f32,
    phase_inc: f32,
}

impl Beeper {
    fn beep(&mut self, ms: i32) {
        self.samples_left = ms * FREQUENCY / 1000;
    }
}

impl AudioCallback for Beeper {
    type Channel = i16;

    fn callback(&mut self, out: &mut [i16]) {
        for sample in out.iter_mut() {
            *sample = if self.samples_left > 0 {
                self.samples_left -= 1;
                self.phase = (self.phase + self.phase_inc) % 1.0;
                if self.phase < 0.5 {
                    5000
                } else {
                    -5000
                }
            } else {
                0
            };
        }
    }
}

fn map_key(key: Keycode) -> Option<u8> {
    Some(match key {
        Keycode::Num1 => 0x0,
        Keycode::Q => 0x1,
        Keycode::W => 0x2,
        Keycode::E => 0x3,
        Keycode::A => 0x4,
        Keycode::S => 0x5,
        Keycode::D => 0x6,
        Keycode::Z => 0x7,
        Keycode::X => 0x8,
        Keycode::C => 0x9,
        Keycode::R => 0xA,
        Keycode::F => 0xB,
        Keycode::V => 0xC,
        Keycode::T => 0xD,
        Keycode::G => 0xE,
        Keycode::B => 0xF,
        _ => return None,
    })
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

    let sdl = sdl2::init()?;
    let window = sdl
        .video()?
        .window(
            &format!("Chip 8 : {}", args[1]),
            Display::WIDTH as u32 * SCALE,
            Display::HEIGHT as u32 * SCALE,
        )
        .position_centered()
        .build()?;
    let mut canvas = window.into_canvas().build()?;
    let mut events = sdl.event_pump()?;

    let mut audio = sdl.audio()?.open_playback(
        None,
        &AudioSpecDesired {
            freq: Some(FREQUENCY),
            channels: Some(1),
            samples: Some(2048),
        },
        |spec| Beeper {
            samples_left: 0,
            phase: 0.0,
            phase_inc: 440.0 / spec.freq as f32,
        },
    )?;
    audio.resume();

    'running: loop {
        std::thread::sleep(Duration::from_millis(16));

        if cpu.tick_timers() {
            audio.lock().beep(BEEP_MS);
        }

        for _ in 0..OPCODES_PER_TICK {
            for event in events.poll_iter() {
                match event {
                    Event::Quit { .. }
                    | Event::KeyDown {
                        keycode: Some(Keycode::Escape),
                        ..
                    } => break 'running,
                    Event::KeyDown {
                        keycode: Some(key), ..
                    } => {
                        if let Some(k) = map_key(key) {
                            cpu.set_key(k, true);
                        }
                    }
                    Event::KeyUp {
                        keycode: Some(key), ..
                    } => {
                        if let Some(k) = map_key(key) {
                            cpu.set_key(k, false);
                        }
                    }
                    _ => {}
                }
            }
            cpu.step();
        }

        // update screen if invalidated
        if cpu.display_mut().take_invalidated() {
            canvas.set_draw_color(Color::BLACK);
            canvas.clear();
            canvas.set_draw_color(Color::WHITE);
            let display = cpu.display();
            for y in 0..Display::HEIGHT {
                for x in 0..Display::WIDTH {
                    if display.get(x, y) {
                        let rect = Rect::new(
                            (x as u32 * SCALE) as i32,
                            (y as u32 * SCALE) as i32,
                            SCALE,
                            SCALE,
                        );
                        canvas.fill_rect(rect)?;
                    }
                }
            }
            canvas.present();
        }
    }

    Ok(())
}
