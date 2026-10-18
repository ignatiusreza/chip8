#include "options.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>

// Windows has no terminal display, so it defaults to the window
#ifdef _WIN32
static const DisplayMode DEFAULT_MODE = DisplayMode::Window;
#else
static const DisplayMode DEFAULT_MODE = DisplayMode::Terminal;
#endif

static void usage(const char *program) {
  std::cout << "Usage : " << program << " [--display tui|window] [--quirks LIST] [--speed N] ROMNAME\n\n"
            << "  --display tui     draw in this terminal" << (DEFAULT_MODE == DisplayMode::Terminal ? " (default)" : "") << "\n"
            << "  --display window  open a separate window" << (DEFAULT_MODE == DisplayMode::Window ? " (default)" : "") << "\n"
            << "  --quirks LIST     comma separated presets (octo, vip, schip) and quirks\n"
            << "                    (shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on,\n"
            << "                    starting from the default\n"
            << "  --speed N         opcodes run per frame\n\n"
            << "Without --quirks or --speed, they are read from the ROM's .cfg file if it has one,\n"
            << "otherwise they default to " << DEFAULT_QUIRKS << " and " << DEFAULT_SPEED << "." << std::endl;
}

static bool parseMode(const std::string &value, DisplayMode &mode) {
  if(value == "tui" || value == "terminal") mode = DisplayMode::Terminal;
  else if(value == "window" || value == "gui") mode = DisplayMode::Window;
  else {
    std::cout << "unknown display \"" << value << "\", expected tui or window" << std::endl;
    return false;
  }
  return true;
}

static bool parseQuirks(const std::string &value, Quirks &quirks) {
  std::string unknown;
  quirks = defaultQuirks();
  if(!quirks.parse(value, unknown)) {
    std::cout << "unknown quirk \"" << unknown << "\", expected octo, vip, schip, shift, load-store, vf-reset, jump-v0, wrap or vblank" << std::endl;
    return false;
  }
  return true;
}

static bool parseSpeedOption(const std::string &value, int &speed) {
  if(!parseSpeed(value, speed)) {
    std::cout << "--speed must be a number from 1 to " << MAX_SPEED << std::endl;
    return false;
  }
  return true;
}

// reads the whole ROM file, printing why it failed if it couldn't
static bool readRom(const std::string &filename, std::vector<unsigned char> &rom) {
  std::ifstream file(filename, std::ios_base::binary | std::ios_base::in);
  if(!file) {
    std::cout << "Could not read " << filename << std::endl;
    return false;
  }

  rom.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  if(file.bad()) {
    std::cout << "Error Reading ROM" << std::endl;
    return false;
  }
  if(rom.size() > CPU::MAX_ROM_SIZE) {
    std::cout << "ROM is too large, at most " << CPU::MAX_ROM_SIZE << " bytes fit in memory" << std::endl;
    return false;
  }
  return true;
}

// Whether argv[i] is option `name`, given as "--name=VALUE" or "--name VALUE"; sets `value`
// and moves `i` past it, or leaves `value` unset (printing why) if it's missing.
static bool option(int argc, char *argv[], int &i, const std::string &name, std::optional<std::string> &value) {
  std::string arg = argv[i];
  if(arg.rfind(name + "=", 0) == 0) {
    value = arg.substr(name.size() + 1);
    return true;
  }
  if(arg != name) return false;
  if(++i < argc) value = argv[i];
  else std::cout << name << " needs a value" << std::endl;
  return true;
}

bool parseOptions(int argc, char *argv[], Options &options, int &status) {
  status = 1;
  options.mode = DEFAULT_MODE;
  // set from the command line, otherwise from the ROM's settings file
  std::optional<Quirks> quirks;
  std::optional<int> speed;
  bool hasRom = false;
  for(int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    std::optional<std::string> value;
    if(option(argc, argv, i, "--display", value)) {
      if(!value || !parseMode(*value, options.mode)) return false;
    } else if(option(argc, argv, i, "--quirks", value)) {
      if(!value || !parseQuirks(*value, quirks.emplace())) return false;
    } else if(option(argc, argv, i, "--speed", value)) {
      if(!value || !parseSpeedOption(*value, speed.emplace())) return false;
    } else if(arg == "-h" || arg == "--help") {
      usage(argv[0]);
      status = 0;
      return false;
    } else if(!hasRom) {
      options.romPath = arg;
      hasRom = true;
    } else {
      std::cout << "unexpected argument \"" << arg << "\"\n\n";
      usage(argv[0]);
      return false;
    }
  }
  if(!hasRom) {
    usage(argv[0]);
    status = 0;
    return false;
  }

  if(!readRom(options.romPath, options.rom)) return false;
  std::string error;
  if(!loadRomSettings(options.romPath, options.settings, error)) {
    std::cout << error << std::endl;
    return false;
  }
  options.quirks = quirks.value_or(options.settings.quirks.value_or(defaultQuirks()));
  options.speed = speed.value_or(options.settings.speed.value_or(DEFAULT_SPEED));
  options.settings.quirks = options.quirks;
  options.settings.speed = options.speed;
  status = 0;
  return true;
}
