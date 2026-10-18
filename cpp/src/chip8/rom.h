#ifndef _CHIP8_CHIP8_ROM_H
#define	_CHIP8_CHIP8_ROM_H

#include <optional>
#include <string>
#include <vector>
#include "cpu.h"

// the settings used for a ROM that doesn't say otherwise
static constexpr const char *DEFAULT_QUIRKS = "octo";
static constexpr int DEFAULT_SPEED = 15;
static constexpr int MAX_SPEED = 10000;

// the default quirks, which quirk lists on the command line and in settings files start
// from, so e.g. "vf-reset" adds that quirk to them
Quirks defaultQuirks();

// What a ROM needs to run well, and how to play it, read from the settings file next to
// it: the ROM's path with its extension replaced by ".cfg". Each line holds "name = value":
//
//   quirks = LIST     the same list as --quirks
//   speed = N         opcodes per frame
//   title = TEXT      the game's name
//   author = TEXT     who made it
//   about = TEXT      what the game is and how to win, a line of text per setting
//   controls = TEXT   a key and what it does, e.g. "A / D: move left / right", per setting
//
// Blank lines and lines starting with "#" are ignored. Quirks and speed that aren't given
// use the defaults.
struct RomSettings {
  std::optional<Quirks> quirks;
  std::optional<int> speed;
  std::string title;
  std::string author;
  std::vector<std::string> about;
  std::vector<std::string> controls;
};

// parses an opcodes per frame count, between 1 and MAX_SPEED
bool parseSpeed(const std::string &value, int &speed);

// parses the text of a settings file; returns false and sets `error` if it is invalid
bool parseRomSettings(const std::string &text, RomSettings &settings, std::string &error);

// the path of the settings file for the ROM at `romPath`
std::string romSettingsPath(const std::string &romPath);

// reads the settings file for the ROM at `romPath`, leaving `settings` empty if there is
// none; returns false and sets `error` if it can't be read or is invalid
bool loadRomSettings(const std::string &romPath, RomSettings &settings, std::string &error);

// A ROM read from a file, with its settings.
struct Rom {
  std::vector<unsigned char> data; // the program, loaded into memory at 0x200
  RomSettings settings;
};

// reads the ROM at `path`, and its settings file if it has one; returns false and sets
// `error` if either can't be read, or the settings are invalid
bool loadRom(const std::string &path, Rom &rom, std::string &error);

#endif	/* _CHIP8_CHIP8_ROM_H */
