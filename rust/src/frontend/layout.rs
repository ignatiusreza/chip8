//! The terminal shows the game as a title line above a bordered box of character cells,
//! with the ROM's how to play next to it or below it, all centred in the terminal.

use chip8::Display;

use super::settings::RomSettings;

/// The gap between the game and how to play beside it.
const PANEL_GAP: usize = 3;
/// How narrow and wide how to play beside the game can be.
const PANEL_MIN_WIDTH: usize = 24;
const PANEL_MAX_WIDTH: usize = 44;

/// The terminal columns the game takes at a scale: 1 for 64x16 cells of 2x4 frame
/// pixels, or 2 for 128x32 cells of 1x2 frame pixels.
pub fn game_columns(scale: usize) -> usize {
    Display::FRAME_WIDTH * scale / 2 + 2
}

/// The terminal rows the game takes at a scale.
pub fn game_rows(scale: usize) -> usize {
    Display::FRAME_HEIGHT * scale / 4 + 3
}

/// Where the game and its how to play go in the terminal; positions are 0-based.
#[derive(Debug, Default, PartialEq, Eq)]
pub struct Layout {
    /// False if the terminal is too small for the game.
    pub fits: bool,
    pub scale: usize,
    /// The game's title line.
    pub game_x: usize,
    pub game_y: usize,
    pub panel_x: usize,
    pub panel_y: usize,
    /// How to play, wrapped to fit.
    pub panel: Vec<String>,
}

impl Layout {
    /// Places the game, as large as possible while how to play still fits next to or
    /// below it, and centres them in a columns x rows terminal.
    pub fn new(columns: usize, rows: usize, settings: &RomSettings) -> Self {
        let has_panel = !how_to_play(settings, PANEL_MAX_WIDTH).is_empty();
        for scale in [2, 1] {
            if !has_panel {
                break;
            }
            let (gw, gh) = (game_columns(scale), game_rows(scale));
            if columns >= gw + PANEL_GAP + PANEL_MIN_WIDTH && rows >= gh {
                let width = PANEL_MAX_WIDTH.min(columns - gw - PANEL_GAP);
                return besides(
                    columns,
                    rows,
                    scale,
                    truncate(how_to_play(settings, width), rows),
                );
            }
            let panel = how_to_play(settings, gw - 2);
            if columns >= gw && rows >= gh + 1 + panel.len() {
                return below(columns, rows, scale, panel);
            }
        }
        // below the normal size game, as much of how to play as fits
        let gh = game_rows(1);
        if has_panel && columns >= game_columns(1) && rows >= gh + 2 {
            let panel = how_to_play(settings, game_columns(1) - 2);
            return below(columns, rows, 1, truncate(panel, rows - gh - 1));
        }
        for scale in [2, 1] {
            if columns >= game_columns(scale) && rows >= game_rows(scale) {
                return below(columns, rows, scale, Vec::new());
            }
        }
        Layout::default()
    }
}

fn besides(columns: usize, rows: usize, scale: usize, panel: Vec<String>) -> Layout {
    let (gw, gh) = (game_columns(scale), game_rows(scale));
    let panel_width = panel.iter().map(|line| text_width(line)).max().unwrap_or(0);
    let (width, height) = (gw + PANEL_GAP + panel_width, gh.max(panel.len()));
    let (x, y) = ((columns - width) / 2, (rows - height) / 2);
    Layout {
        fits: true,
        scale,
        game_x: x,
        game_y: y + (height - gh) / 2,
        panel_x: x + gw + PANEL_GAP,
        panel_y: y + (height - panel.len()) / 2,
        panel,
    }
}

fn below(columns: usize, rows: usize, scale: usize, panel: Vec<String>) -> Layout {
    let (gw, gh) = (game_columns(scale), game_rows(scale));
    let height = gh + if panel.is_empty() { 0 } else { 1 + panel.len() };
    let (x, y) = ((columns - gw) / 2, (rows - height) / 2);
    Layout {
        fits: true,
        scale,
        game_x: x,
        game_y: y,
        // how to play lines up with the inside of the game's border
        panel_x: x + 1,
        panel_y: y + gh + 1,
        panel,
    }
}

/// The ROM's title, author, about, controls, and the quirks and speed it runs with, as
/// lines of at most `width` characters, with blank lines between them.
pub fn how_to_play(settings: &RomSettings, width: usize) -> Vec<String> {
    let mut lines = Vec::new();
    let mut section = |texts: &[String], indent: usize| {
        if texts.is_empty() {
            return;
        }
        if !lines.is_empty() {
            lines.push(String::new());
        }
        for text in texts {
            lines.extend(wrap(text, width, indent));
        }
    };
    let heading: Vec<String> = [
        settings.title.clone(),
        settings
            .author
            .as_ref()
            .map(|author| format!("by {author}")),
    ]
    .into_iter()
    .flatten()
    .collect();
    section(&heading, 0);
    section(&settings.about, 0);
    if !settings.controls.is_empty() {
        let controls: Vec<String> = std::iter::once("Controls".to_string())
            .chain(settings.controls.iter().cloned())
            .collect();
        section(&controls, 2);
    }
    let running: Vec<String> = [
        settings.quirks.map(|quirks| format!("Quirks: {quirks}")),
        settings
            .speed
            .map(|speed| format!("Speed: {speed} opcodes per frame")),
    ]
    .into_iter()
    .flatten()
    .collect();
    section(&running, 2);
    lines
}

/// Breaks text into lines of at most `width` characters at spaces, indenting the lines
/// after the first, and splitting words that are too long.
pub fn wrap(text: &str, width: usize, indent: usize) -> Vec<String> {
    let mut lines = Vec::new();
    let mut line = String::new();
    for word in text.split_whitespace() {
        if line.is_empty() {
            line = word.to_string();
        } else if text_width(&line) + 1 + text_width(word) <= width {
            line = format!("{line} {word}");
        } else {
            lines.push(std::mem::take(&mut line));
            line = format!("{}{word}", " ".repeat(indent));
        }
        while text_width(&line) > width {
            let head: String = line.chars().take(width).collect();
            let rest: String = line.chars().skip(width).collect();
            lines.push(head);
            line = format!("{}{rest}", " ".repeat(indent));
        }
    }
    if !line.is_empty() {
        lines.push(line);
    }
    lines
}

/// The width of a line in characters.
pub fn text_width(text: &str) -> usize {
    text.chars().count()
}

fn truncate(mut lines: Vec<String>, n: usize) -> Vec<String> {
    lines.truncate(n);
    lines
}

#[cfg(test)]
mod tests {
    use super::*;

    fn br8kout() -> RomSettings {
        RomSettings {
            title: Some("Br8kout".into()),
            author: Some("SharpenedSpoon".into()),
            about: vec![
                "Breakout: bounce the ball off your paddle to break all the bricks.".into(),
            ],
            controls: vec!["A / D: move the paddle left / right".into()],
            ..RomSettings::default()
        }
    }

    fn panel_width(l: &Layout) -> usize {
        l.panel
            .iter()
            .map(|line| text_width(line))
            .max()
            .unwrap_or(0)
    }

    #[test]
    fn how_to_play_lines() {
        assert_eq!(
            how_to_play(&br8kout(), 30),
            [
                "Br8kout",
                "by SharpenedSpoon",
                "",
                "Breakout: bounce the ball off",
                "your paddle to break all the",
                "bricks.",
                "",
                "Controls",
                "A / D: move the paddle left /",
                "  right",
            ]
        );
        assert!(how_to_play(&RomSettings::default(), 30).is_empty());
    }

    #[test]
    fn how_to_play_shows_quirks_and_speed() {
        let settings = RomSettings {
            quirks: Some("octo,vf-reset".parse().unwrap()),
            speed: Some(7),
            ..br8kout()
        };
        let lines = how_to_play(&settings, 40);
        assert_eq!(
            lines[lines.len() - 3..],
            ["", "Quirks: octo,vf-reset", "Speed: 7 opcodes per frame"]
        );
    }

    #[test]
    fn wrap_splits_long_words() {
        assert_eq!(wrap("abcdefgh", 4, 2), ["abcd", "  ef", "  gh"]);
        assert_eq!(text_width("█▀ ab"), 5);
    }

    #[test]
    fn picks_the_largest_game_that_fits_with_how_to_play() {
        for (columns, rows, scale, besides) in [
            (200, 50, 2, true),  // 130 + 3 + 24 columns fit beside the large game
            (140, 60, 2, false), // the large game is too wide to fit it beside, but it fits below
            (100, 30, 1, true),  // too small for the large game, so the normal one with it beside
            (70, 40, 1, false),  // too narrow to put it beside, so below
            (66, 22, 1, false),  // below, cut short
        ] {
            let l = Layout::new(columns, rows, &br8kout());
            assert!(l.fits, "{columns}x{rows}");
            assert_eq!(l.scale, scale, "{columns}x{rows}");
            assert_eq!(l.panel_x > l.game_x + 1, besides, "{columns}x{rows}");
            assert!((l.game_y + game_rows(l.scale)).max(l.panel_y + l.panel.len()) <= rows);
            assert!((l.game_x + game_columns(l.scale)).max(l.panel_x + panel_width(&l)) <= columns);
        }
    }

    #[test]
    fn centres() {
        // the normal size game alone, 66x19, in the middle of 100x29
        let l = Layout::new(100, 29, &RomSettings::default());
        assert_eq!((l.scale, l.game_x, l.game_y), (1, 17, 5));
        assert!(l.panel.is_empty());

        // beside: the game and how to play, 66 + 3 + 29 columns, centred together
        let l = Layout::new(100, 29, &br8kout());
        assert_eq!((l.game_x, l.panel_x, l.game_y, l.panel_y), (1, 70, 5, 9));
    }

    #[test]
    fn too_small() {
        assert!(!Layout::new(65, 40, &br8kout()).fits);
        assert!(!Layout::new(66, 18, &RomSettings::default()).fits);
    }
}
