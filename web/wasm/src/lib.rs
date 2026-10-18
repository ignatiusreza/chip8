//! The CHIP-8 core as a WebAssembly module, driven by `web/static/app.js`.
//!
//! It holds a single emulator and exports a few plain functions, so the page needs no
//! generated glue:
//!
//! - `buffer(len)` returns where the page writes `len` bytes of input, for `load`.
//! - `load(rom_len, quirks_len, seed)` starts the ROM in the first `rom_len` bytes of the
//!   input, with the quirk list in the next `quirks_len` bytes (UTF-8, as `--quirks`
//!   takes it, starting from no quirks), and `seed` for the random numbers. It returns
//!   0, or an `Error`.
//! - `set_key(key, pressed)` presses or releases a keypad key, 0x0 - 0xF.
//! - `run_frame(speed)` runs one 1/60s frame of `speed` opcodes and returns the `FLAG_`
//!   bits.
//! - `frame()` returns the 128x64 frame as `FRAME_BYTES` bytes, a bit per pixel, row by
//!   row, the leftmost pixel in the highest bit; low resolution pixels cover 2x2 of them.

use std::cell::RefCell;

use chip8::{Cpu, Display, Quirks};

/// The size of the packed frame `frame` returns.
pub const FRAME_BYTES: usize = Display::FRAME_LENGTH / 8;

/// `run_frame` flags: the display changed (so `frame` has a new picture), the beeper
/// plays, the display is in high resolution, and the program exited.
pub const FLAG_CHANGED: u32 = 1;
pub const FLAG_SOUND: u32 = 2;
pub const FLAG_HIRES: u32 = 4;
pub const FLAG_EXITED: u32 = 8;

/// Why `load` failed.
#[repr(i32)]
pub enum Error {
    RomTooLarge = 1,
    BadQuirks = 2,
    BadLength = 3,
}

struct State {
    cpu: Cpu,
    frame: [u8; FRAME_BYTES],
    input: Vec<u8>,
}

thread_local! {
    static STATE: RefCell<State> = RefCell::new(State {
        cpu: Cpu::with_seed(1),
        frame: [0; FRAME_BYTES],
        input: Vec::new(),
    });
}

#[no_mangle]
pub extern "C" fn buffer(len: usize) -> *mut u8 {
    STATE.with_borrow_mut(|state| {
        state.input.resize(len, 0);
        state.input.as_mut_ptr()
    })
}

#[no_mangle]
pub extern "C" fn load(rom_len: usize, quirks_len: usize, seed: u32) -> i32 {
    STATE.with_borrow_mut(|state| {
        let Some((rom, rest)) = state.input.split_at_checked(rom_len) else {
            return Error::BadLength as i32;
        };
        let Some(quirks) = rest.get(..quirks_len) else {
            return Error::BadLength as i32;
        };
        let Some(quirks) = std::str::from_utf8(quirks)
            .ok()
            .and_then(|list| Quirks::default().apply(list).ok())
        else {
            return Error::BadQuirks as i32;
        };
        let mut cpu = Cpu::with_seed(seed);
        cpu.set_quirks(quirks);
        if cpu.load(rom).is_err() {
            return Error::RomTooLarge as i32;
        }
        state.cpu = cpu;
        0
    })
}

#[no_mangle]
pub extern "C" fn set_key(key: u32, pressed: u32) {
    if key < 16 {
        STATE.with_borrow_mut(|state| state.cpu.set_key(key as u8, pressed != 0));
    }
}

#[no_mangle]
pub extern "C" fn run_frame(speed: u32) -> u32 {
    STATE.with_borrow_mut(|state| {
        let mut flags = 0;
        if state.cpu.tick_timers() {
            flags |= FLAG_SOUND;
        }
        for _ in 0..speed {
            state.cpu.step();
        }
        let display = state.cpu.display_mut();
        if display.take_invalidated() {
            flags |= FLAG_CHANGED;
            state.frame = [0; FRAME_BYTES];
            for y in 0..Display::FRAME_HEIGHT {
                for x in 0..Display::FRAME_WIDTH {
                    if display.pixel(x, y) {
                        let i = y * Display::FRAME_WIDTH + x;
                        state.frame[i / 8] |= 0x80 >> (i % 8);
                    }
                }
            }
        }
        if display.hires() {
            flags |= FLAG_HIRES;
        }
        if state.cpu.exited() {
            flags |= FLAG_EXITED;
        }
        flags
    })
}

#[no_mangle]
pub extern "C" fn frame() -> *const u8 {
    STATE.with_borrow(|state| state.frame.as_ptr())
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Loads a program through the exported functions, as the page does.
    fn start(program: &[u16], quirks: &str) -> i32 {
        let rom: Vec<u8> = program.iter().flat_map(|op| op.to_be_bytes()).collect();
        let input = [rom.as_slice(), quirks.as_bytes()].concat();
        let at = buffer(input.len());
        // SAFETY: buffer just made room for input.len() bytes at `at`
        unsafe { std::ptr::copy_nonoverlapping(input.as_ptr(), at, input.len()) };
        load(rom.len(), quirks.len(), 1)
    }

    fn frame_bytes() -> [u8; FRAME_BYTES] {
        // SAFETY: frame points at the FRAME_BYTES bytes of the state's frame
        unsafe { *(frame() as *const [u8; FRAME_BYTES]) }
    }

    #[test]
    fn runs_a_frame_and_packs_the_display() {
        // draw the top row of font "0" (4 pixels) at (2, 0), then loop
        assert_eq!(start(&[0x6002, 0x6100, 0xA000, 0xD011, 0x1208], "octo"), 0);
        let flags = run_frame(10);
        assert_eq!(flags & (FLAG_CHANGED | FLAG_HIRES), FLAG_CHANGED);
        // low resolution pixels 2..5 are frame pixels 4..11, on frame rows 0 and 1
        let frame = frame_bytes();
        assert_eq!(&frame[..2], &[0x0F, 0xF0]);
        assert_eq!(&frame[16..18], &[0x0F, 0xF0]);
        assert_eq!(
            frame[2..16]
                .iter()
                .chain(&frame[18..])
                .filter(|&&b| b != 0)
                .count(),
            0
        );

        // nothing changes in the next frame
        assert_eq!(run_frame(10) & FLAG_CHANGED, 0);
    }

    #[test]
    fn reports_sound_hires_and_exit() {
        // high resolution, sound for 2 frames, exit
        assert_eq!(start(&[0x00FF, 0x6002, 0xF018, 0x00FD], "schip"), 0);
        let flags = run_frame(10);
        assert_eq!(flags & (FLAG_HIRES | FLAG_EXITED), FLAG_HIRES | FLAG_EXITED);
        assert_ne!(run_frame(10) & FLAG_SOUND, 0);
    }

    #[test]
    fn keys_reach_the_cpu() {
        // wait for a key into V0, then draw font V0 at (V0, V0)
        assert_eq!(start(&[0xF00A, 0xF029, 0xD005, 0x1206], "octo"), 0);
        assert_eq!(run_frame(10) & FLAG_CHANGED, FLAG_CHANGED); // cleared on start
        assert_eq!(run_frame(10) & FLAG_CHANGED, 0);
        set_key(1, 1);
        assert_eq!(run_frame(10) & FLAG_CHANGED, 0); // FX0A waits for the release
        set_key(1, 0);
        assert_eq!(run_frame(10) & FLAG_CHANGED, FLAG_CHANGED);
        // font "1" starts 0x20, a pixel at x 2 of the sprite: at (3, 1) in low resolution,
        // which is frame pixels 6 and 7 on frame rows 2 and 3, 16 bytes a row
        let frame = frame_bytes();
        assert_eq!((frame[32], frame[48]), (0x03, 0x03));
    }

    #[test]
    fn rejects_bad_input() {
        assert_eq!(start(&[0x1200], "bogus"), Error::BadQuirks as i32);
        assert_eq!(start(&[0; 0x701], "octo"), Error::RomTooLarge as i32);
        buffer(4);
        assert_eq!(load(4, 1, 1), Error::BadLength as i32);
    }
}
