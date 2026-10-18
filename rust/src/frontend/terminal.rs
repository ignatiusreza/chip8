use std::io::{self, IsTerminal, Stdout, Write};
use std::thread;
use std::time::{Duration, Instant};

use chip8::Display;
use crossterm::cursor::{Hide, MoveTo, Show};
use crossterm::event::{
    self, Event, KeyCode, KeyEventKind, KeyModifiers, KeyboardEnhancementFlags,
    PopKeyboardEnhancementFlags, PushKeyboardEnhancementFlags,
};
use crossterm::terminal::{
    self, Clear, ClearType, EnterAlternateScreen, LeaveAlternateScreen, SetTitle,
};
use crossterm::{execute, queue};

use super::input::Keys;
use super::layout::{game_columns, game_rows, Layout};
use super::settings::RomSettings;
use super::{Result, Screen};

const FRAME: Duration = Duration::from_nanos(1_000_000_000 / 60);

// Most terminals only send key presses, repeated while the key is held. Without
// release events, a key counts as held for this long after its last press, which
// bridges the gap before the terminal starts repeating it.
const HOLD: Duration = Duration::from_millis(200);

// character for each CHIP-8 keypad value, laid out like the COSMAC
// VIP keypad on the left of a QWERTY keyboard:
//   1 2 3 4        1 2 3 C
//   Q W E R   ->   4 5 6 D
//   A S D F        7 8 9 E
//   Z X C V        A 0 B F
// so W A S D are 5 7 8 9, the up, left, down and right of most games
const KEYMAP: [char; 16] = [
    'x', '1', '2', '3', // 0 1 2 3
    'q', 'w', 'e', 'a', // 4 5 6 7
    's', 'd', 'z', 'c', // 8 9 A B
    '4', 'r', 'f', 'v', // C D E F
];

// the display plus a one character border, and a title line above it
/// The braille character showing the 2x4 high resolution pixels at (x, y).
fn braille(display: &Display, x: usize, y: usize) -> char {
    // the bit of each dot, by column then row
    const DOTS: [[u32; 4]; 2] = [[0x01, 0x02, 0x04, 0x40], [0x08, 0x10, 0x20, 0x80]];
    let mut bits = 0;
    for (dx, column) in DOTS.iter().enumerate() {
        for (dy, bit) in column.iter().enumerate() {
            if display.pixel(x + dx, y + dy) {
                bits |= bit;
            }
        }
    }
    char::from_u32(0x2800 + bits).unwrap_or(' ')
}

/// The half block showing two pixels, one above the other.
fn half_block(top: bool, bottom: bool) -> char {
    match (top, bottom) {
        (true, true) => '█',
        (true, false) => '▀',
        (false, true) => '▄',
        (false, false) => ' ',
    }
}

/// The terminal the emulator was started from, showing the display and the ROM's how to
/// play, centred, and reading the keyboard.
pub struct Terminal {
    out: Stdout,
    title: String,
    how_to: RomSettings,
    layout: Layout,
    // whether the terminal reports key releases (kitty keyboard protocol)
    release_events: bool,
    held: [bool; 16],
    last_press: [Option<Instant>; 16],
    quit: bool,
    resized: bool,
    next_frame: Instant,
}

impl Terminal {
    /// Switches the terminal to raw mode and an alternate screen, which are restored
    /// when the Terminal is dropped. `how_to` is the ROM's settings, whose how to play is
    /// shown next to the game.
    pub fn new(title: &str, how_to: &RomSettings) -> Result<Self> {
        if !io::stdin().is_terminal() || !io::stdout().is_terminal() {
            return Err(
                "the terminal display needs an interactive terminal, try --display window".into(),
            );
        }

        terminal::enable_raw_mode()?;
        // from here on, dropping term restores the terminal, also on error
        let mut term = Terminal {
            out: io::stdout(),
            title: title.to_string(),
            how_to: how_to.clone(),
            layout: Layout::default(),
            release_events: false,
            held: [false; 16],
            last_press: [None; 16],
            quit: false,
            resized: true,
            next_frame: Instant::now(),
        };
        execute!(term.out, EnterAlternateScreen, Hide, SetTitle(title))?;
        if terminal::supports_keyboard_enhancement().unwrap_or(false) {
            execute!(
                term.out,
                PushKeyboardEnhancementFlags(
                    KeyboardEnhancementFlags::DISAMBIGUATE_ESCAPE_CODES
                        | KeyboardEnhancementFlags::REPORT_EVENT_TYPES
                )
            )?;
            term.release_events = true;
        }
        Ok(term)
    }

    fn handle(&mut self, event: Event) {
        let key = match event {
            Event::Key(key) => key,
            Event::Resize(..) => {
                self.resized = true;
                return;
            }
            _ => return,
        };

        let pressed = key.kind != KeyEventKind::Release;
        match key.code {
            KeyCode::Esc => self.quit |= pressed,
            KeyCode::Char('c') if key.modifiers.contains(KeyModifiers::CONTROL) => self.quit = true,
            KeyCode::Char(c) => {
                if let Some(k) = KEYMAP.iter().position(|&m| m == c.to_ascii_lowercase()) {
                    self.held[k] = pressed;
                    if pressed {
                        self.last_press[k] = Some(Instant::now());
                    }
                }
            }
            _ => {}
        }
    }

    /// Draws the whole screen; each frame overwrites the last, so the terminal only
    /// needs clearing when resized.
    /// Draws the display. When resized, lays the game and how to play out again and
    /// draws the title, border and how to play, which only change then.
    fn render(&mut self, display: &Display, resized: bool) -> io::Result<()> {
        if resized {
            let (columns, rows) = terminal::size()?;
            self.layout = Layout::new(columns.into(), rows.into(), &self.how_to);
            queue!(self.out, Clear(ClearType::All))?;
        }
        let l = &self.layout;
        if !l.fits {
            if resized {
                queue!(self.out, MoveTo(0, 0))?;
                write!(
                    self.out,
                    "Terminal too small, needs {}x{}",
                    game_columns(1),
                    game_rows(1)
                )?;
            }
            return self.out.flush();
        }

        let (cells_wide, cells_high) = (game_columns(l.scale) - 2, game_rows(l.scale) - 3);
        let move_to = |x: usize, y: usize| MoveTo(x as u16, y as u16);
        if resized {
            let title: String = format!("{} (Esc to quit)", self.title)
                .chars()
                .take(cells_wide + 2)
                .collect();
            let border = "─".repeat(cells_wide);
            queue!(self.out, move_to(l.game_x, l.game_y))?;
            write!(self.out, "{title}")?;
            queue!(self.out, move_to(l.game_x, l.game_y + 1))?;
            write!(self.out, "┌{border}┐")?;
            for row in 0..cells_high {
                queue!(self.out, move_to(l.game_x, l.game_y + 2 + row))?;
                write!(self.out, "│")?;
                queue!(
                    self.out,
                    move_to(l.game_x + cells_wide + 1, l.game_y + 2 + row)
                )?;
                write!(self.out, "│")?;
            }
            queue!(self.out, move_to(l.game_x, l.game_y + cells_high + 2))?;
            write!(self.out, "└{border}┘")?;
            for (i, line) in l.panel.iter().enumerate() {
                queue!(self.out, move_to(l.panel_x, l.panel_y + i))?;
                write!(self.out, "{line}")?;
            }
        }

        for row in 0..cells_high {
            queue!(self.out, move_to(l.game_x + 1, l.game_y + 2 + row))?;
            let line: String = (0..cells_wide)
                .map(|col| {
                    if l.scale == 2 {
                        // two frame pixels, one above the other
                        half_block(display.pixel(col, row * 2), display.pixel(col, row * 2 + 1))
                    } else if display.hires() {
                        braille(display, col * 2, row * 4)
                    } else {
                        // two low resolution pixels, one above the other
                        half_block(display.get(col, row * 2), display.get(col, row * 2 + 1))
                    }
                })
                .collect();
            write!(self.out, "{line}")?;
        }
        self.out.flush()
    }
}

impl Screen for Terminal {
    fn keys(&mut self) -> Keys {
        while event::poll(Duration::ZERO).unwrap_or(false) {
            match event::read() {
                Ok(event) => self.handle(event),
                Err(_) => break,
            }
        }

        let now = Instant::now();
        let down = std::array::from_fn(|k| {
            if self.release_events {
                self.held[k]
            } else {
                self.last_press[k].is_some_and(|t| now - t < HOLD)
            }
        });
        Keys {
            down,
            quit: self.quit,
        }
    }

    /// Redraws the display if it changed or the terminal was resized, then waits
    /// for the next 1/60s frame.
    fn draw(&mut self, display: &mut Display) -> Result<()> {
        let resized = std::mem::take(&mut self.resized);
        if display.take_invalidated() || resized {
            self.render(display, resized)?;
        }

        let now = Instant::now();
        if self.next_frame > now {
            thread::sleep(self.next_frame - now);
            self.next_frame += FRAME;
        } else {
            // running behind, don't try to catch up
            self.next_frame = now + FRAME;
        }
        Ok(())
    }
}

impl Drop for Terminal {
    fn drop(&mut self) {
        if self.release_events {
            let _ = execute!(self.out, PopKeyboardEnhancementFlags);
        }
        let _ = execute!(self.out, Show, LeaveAlternateScreen);
        let _ = terminal::disable_raw_mode();
    }
}
