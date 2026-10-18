#ifndef _CHIP8_FRONTEND_OPTIONS_H
#define	_CHIP8_FRONTEND_OPTIONS_H

#include <string>
#include <vector>
#include "../core/cpu.h"
#include "../core/settings.h"

// Where the display is shown and the keyboard read from.
enum class DisplayMode {
  Terminal, // the terminal the emulator was started from
  Window,   // a separate desktop window (SDL video must be initialized first)
};

// What to run and how: the command line, with the ROM and its settings file.
struct Options {
  std::string romPath;
  std::vector<unsigned char> rom;
  DisplayMode mode;
  // the quirks and speed the ROM runs with: from the command line, otherwise its settings
  // file, otherwise the defaults
  Quirks quirks;
  int speed;
  // the ROM's settings file, with those quirks and speed, for the terminal to show
  RomSettings settings;
};

// Parses the command line, then reads the ROM and its settings file. Returns false if
// there is nothing to run, after printing the usage (`status` 0) or why (`status` 1).
bool parseOptions(int argc, char *argv[], Options &options, int &status);

#endif	/* _CHIP8_FRONTEND_OPTIONS_H */
