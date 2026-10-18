//! What a ROM needs to run well, and how to play it, read from the settings file next to
//! it: the ROM's path with its extension replaced by ".cfg". Each line holds "name = value":
//!
//! - `quirks = LIST`: the same list as --quirks
//! - `speed = N`: opcodes per frame
//! - `title = TEXT`: the game's name
//! - `author = TEXT`: who made it
//! - `about = TEXT`: what the game is and how to win, a line of text per setting
//! - `controls = TEXT`: a key and what it does, e.g. "A / D: move left / right", per setting
//!
//! Blank lines and lines starting with "#" are ignored. Quirks and speed that aren't given
//! use the defaults.

use std::path::{Path, PathBuf};

use chip8::Quirks;

/// The quirks used for a ROM that doesn't say otherwise.
pub const DEFAULT_QUIRKS: &str = "octo";
/// The opcodes run per frame for a ROM that doesn't say otherwise.
pub const DEFAULT_SPEED: u32 = 15;
pub const MAX_SPEED: u32 = 10000;

#[derive(Debug, Default, PartialEq, Eq)]
pub struct RomSettings {
    pub quirks: Option<Quirks>,
    pub speed: Option<u32>,
    pub title: Option<String>,
    pub author: Option<String>,
    pub about: Vec<String>,
    pub controls: Vec<String>,
}

/// The default quirks, which quirk lists on the command line and in settings files start
/// from, so e.g. "vf-reset" adds that quirk to them.
pub fn default_quirks() -> Quirks {
    DEFAULT_QUIRKS
        .parse()
        .expect("the default quirks are valid")
}

/// Parses a quirk list, starting from the default quirks.
pub fn parse_quirks(list: &str) -> Result<Quirks, String> {
    default_quirks().apply(list)
}

/// Parses an opcodes per frame count, between 1 and `MAX_SPEED`.
pub fn parse_speed(value: &str) -> Result<u32, String> {
    value
        .parse()
        .ok()
        .filter(|speed| (1..=MAX_SPEED).contains(speed))
        .ok_or_else(|| format!("speed must be a number from 1 to {MAX_SPEED}"))
}

/// Parses the text of a settings file.
pub fn parse(text: &str) -> Result<RomSettings, String> {
    let mut settings = RomSettings::default();
    for (number, line) in text.lines().enumerate() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let at = |error: String| format!("line {}: {error}", number + 1);
        let (name, value) = line
            .split_once('=')
            .ok_or_else(|| at("expected \"name = value\"".into()))?;
        let value = value.trim();
        match name.trim() {
            "quirks" => settings.quirks = Some(parse_quirks(value).map_err(at)?),
            "speed" => settings.speed = Some(parse_speed(value).map_err(at)?),
            "title" => settings.title = Some(value.into()),
            "author" => settings.author = Some(value.into()),
            "about" => settings.about.push(value.into()),
            "controls" => settings.controls.push(value.into()),
            name => {
                return Err(at(format!(
                    "unknown setting {name:?}, expected quirks, speed, title, author, about or controls"
                )))
            }
        }
    }
    Ok(settings)
}

/// The path of the settings file for the ROM at `rom_path`.
pub fn path(rom_path: &Path) -> PathBuf {
    rom_path.with_extension("cfg")
}

/// Reads the settings file for the ROM at `rom_path`, if it has one.
pub fn load(rom_path: &Path) -> Result<RomSettings, String> {
    let path = path(rom_path);
    match std::fs::read_to_string(&path) {
        Ok(text) => parse(&text).map_err(|e| format!("{}, {e}", path.display())),
        // the ROM runs with the defaults
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(RomSettings::default()),
        Err(e) => Err(format!("Could not read {}: {e}", path.display())),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_quirks_and_speed() {
        let settings = parse("# Some Game\n\n  quirks = vip,wrap \r\nspeed=200\n").unwrap();
        let quirks = settings.quirks.unwrap();
        assert!(quirks.vblank && quirks.wrap);
        assert_eq!(settings.speed, Some(200));
    }

    #[test]
    fn quirks_start_from_the_default() {
        let quirks = parse("quirks = vf-reset\n").unwrap().quirks.unwrap();
        assert!(quirks.shift && quirks.load_store && quirks.wrap && quirks.jump_v0);
        assert!(quirks.vf_reset);

        let quirks = parse("quirks = schip,shift\n").unwrap().quirks.unwrap();
        assert!(!quirks.load_store && !quirks.wrap && !quirks.jump_v0 && quirks.shift);
    }

    #[test]
    fn leaves_missing_settings_unset() {
        let settings = parse("speed = 7\n").unwrap();
        assert_eq!(settings.quirks, None);
        assert_eq!(settings.speed, Some(7));
    }

    #[test]
    fn rejects_invalid_settings() {
        assert_eq!(
            parse("quirks = octo\nspeed = fast\n").unwrap_err(),
            "line 2: speed must be a number from 1 to 10000"
        );
        assert!(parse("quirks = bogus\n")
            .unwrap_err()
            .starts_with("line 1: unknown quirk \"bogus\""));
        assert_eq!(
            parse("colour = green\n").unwrap_err(),
            "line 1: unknown setting \"colour\", expected quirks, speed, title, author, about or controls"
        );
        assert_eq!(
            parse("speed\n").unwrap_err(),
            "line 1: expected \"name = value\""
        );
        assert!(parse("speed = 0\n").is_err());
    }

    #[test]
    fn parses_how_to_play() {
        let settings = parse(concat!(
            "title = Br8kout\nauthor = SharpenedSpoon\n",
            "about = Break the bricks.\nabout = Don't miss the ball.\n",
            "controls = A / D: move left / right\ncontrols = E: serve\n",
        ))
        .unwrap();
        assert_eq!(settings.title.as_deref(), Some("Br8kout"));
        assert_eq!(settings.author.as_deref(), Some("SharpenedSpoon"));
        assert_eq!(
            settings.about,
            ["Break the bricks.", "Don't miss the ball."]
        );
        assert_eq!(settings.controls, ["A / D: move left / right", "E: serve"]);
        assert_eq!((settings.quirks, settings.speed), (None, None));
    }

    #[test]
    fn path_replaces_the_extension() {
        assert_eq!(
            path(Path::new("roms/br8kout.ch8")),
            Path::new("roms/br8kout.cfg")
        );
        assert_eq!(path(Path::new("roms/pong")), Path::new("roms/pong.cfg"));
        assert_eq!(
            path(Path::new("my.roms/pong")),
            Path::new("my.roms/pong.cfg")
        );
    }
}
