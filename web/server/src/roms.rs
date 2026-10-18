//! The bundled ROMs and what the player shows for each, from their settings files.

use std::path::Path;

use chip8::{rom, RomSettings};
use serde::Serialize;

/// A ROM, with its how to play and the quirks and speed it runs with.
#[derive(Debug, Serialize)]
pub struct Rom {
    /// The file name without `.ch8`, which is also how the page asks for it.
    pub name: String,
    pub title: String,
    pub author: Option<String>,
    pub about: Vec<String>,
    pub controls: Vec<String>,
    /// A complete quirk list, starting with a preset, so it means the same with or
    /// without the default quirks to start from.
    pub quirks: String,
    pub speed: u32,
}

/// Reads the `.ch8` files in `dir` and their settings files, ordered by title.
pub fn load(dir: &Path) -> Result<Vec<Rom>, String> {
    let entries =
        std::fs::read_dir(dir).map_err(|e| format!("Could not read {}: {e}", dir.display()))?;
    let mut roms = Vec::new();
    for entry in entries {
        let path = entry.map_err(|e| e.to_string())?.path();
        if path.extension().is_none_or(|ext| ext != "ch8") {
            continue;
        }
        let Some(name) = path.file_stem().and_then(|stem| stem.to_str()) else {
            continue;
        };
        let rom_settings = RomSettings::load(&path)?;
        roms.push(Rom {
            name: name.to_string(),
            title: rom_settings.title.unwrap_or_else(|| name.to_string()),
            author: rom_settings.author,
            about: rom_settings.about,
            controls: rom_settings.controls,
            quirks: rom_settings
                .quirks
                .unwrap_or_else(rom::default_quirks)
                .to_string(),
            speed: rom_settings.speed.unwrap_or(rom::DEFAULT_SPEED),
        });
    }
    roms.sort_by_key(|rom| rom.title.to_lowercase());
    Ok(roms)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loads_the_bundled_roms() {
        let roms = load(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../../roms")).unwrap();
        assert_eq!(roms.len(), 24);
        let br8kout = roms.iter().find(|rom| rom.name == "br8kout").unwrap();
        assert_eq!(br8kout.title, "Br8kout");
        assert_eq!(br8kout.quirks, "octo");
        assert_eq!(br8kout.speed, 7);
        let tick = roms.iter().find(|rom| rom.name == "tick").unwrap();
        assert_eq!((tick.quirks.as_str(), tick.speed), ("vip,wrap", 12));
    }
}
