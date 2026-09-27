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
use super::{Result, Screen};

const FRAME: Duration = Duration::from_nanos(1_000_000_000 / 60);

// Most terminals only send key presses, repeated while the key is held. Without
// release events, a key counts as held for this long after its last press, which
// bridges the gap before the terminal starts repeating it.
const HOLD: Duration = Duration::from_millis(200);

// CHIP-8 keypad value for each index, mapped onto the left side of a QWERTY keyboard
const KEYMAP: [char; 16] = [
    '1', 'q', 'w', 'e', // 0 1 2 3
    'a', 's', 'd', 'z', // 4 5 6 7
    'x', 'c', 'r', 'f', // 8 9 A B
    'v', 't', 'g', 'b', // C D E F
];

// the display plus a one character border, and a title line above it
const COLUMNS: u16 = Display::WIDTH as u16 + 2;
const ROWS: u16 = Display::HEIGHT as u16 / 2 + 3;

/// The terminal the emulator was started from, showing the 64x32 display as 64x16
/// half-block characters and reading the keyboard.
pub struct Terminal {
    out: Stdout,
    title: String,
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
    /// when the Terminal is dropped.
    pub fn new(title: &str) -> Result<Self> {
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
    fn render(&mut self, display: &Display, clear: bool) -> io::Result<()> {
        if clear {
            queue!(self.out, Clear(ClearType::All))?;
        }
        queue!(self.out, MoveTo(0, 0))?;
        let (columns, rows) = terminal::size()?;
        if columns < COLUMNS || rows < ROWS {
            write!(self.out, "Terminal too small, needs {COLUMNS}x{ROWS}")?;
            return self.out.flush();
        }

        write!(self.out, "{} (Esc to quit)", self.title)?;
        queue!(self.out, MoveTo(0, 1))?;
        write!(self.out, "┌{}┐", "─".repeat(Display::WIDTH))?;
        // each character cell holds two pixels, one above the other
        for row in 0..Display::HEIGHT / 2 {
            queue!(self.out, MoveTo(0, row as u16 + 2))?;
            let line: String = (0..Display::WIDTH)
                .map(
                    |x| match (display.get(x, row * 2), display.get(x, row * 2 + 1)) {
                        (true, true) => '█',
                        (true, false) => '▀',
                        (false, true) => '▄',
                        (false, false) => ' ',
                    },
                )
                .collect();
            write!(self.out, "│{line}│")?;
        }
        queue!(self.out, MoveTo(0, ROWS - 1))?;
        write!(self.out, "└{}┘", "─".repeat(Display::WIDTH))?;
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
