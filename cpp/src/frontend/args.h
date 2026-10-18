#ifndef _CHIP8_FRONTEND_ARGS_H
#define	_CHIP8_FRONTEND_ARGS_H

#include <optional>
#include <string>
#include "../chip8/cpu.h"

// Where the display is shown and the keyboard read from.
enum class DisplayMode {
  Terminal, // the terminal the emulator was started from
  Window,   // a separate desktop window (SDL video must be initialized first)
};

// The command line: the ROM to run and how.
struct Args {
  std::string romPath;
  DisplayMode mode;
  // the quirks and speed to run the ROM with, if given, instead of the ones from its
  // settings file
  std::optional<Quirks> quirks;
  std::optional<int> speed;
};

// Parses the command line. Returns false if there is nothing to run, after printing the
// usage (`status` 0) or why (`status` 1).
bool parseArgs(int argc, char *argv[], Args &args, int &status);

#endif	/* _CHIP8_FRONTEND_ARGS_H */
