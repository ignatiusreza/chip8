#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "core/cpu.h"
#include "frontend/emulator.h"
#include "frontend/settings.h"

// reads the whole ROM file, printing why it failed if it couldn't
static bool readRom(const char *filename, std::vector<unsigned char> &rom) {
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

// Windows has no terminal display, so it defaults to the window
#ifdef _WIN32
static const DisplayMode DEFAULT_MODE = DisplayMode::Window;
#else
static const DisplayMode DEFAULT_MODE = DisplayMode::Terminal;
#endif

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
  if(!quirks.parse(value, unknown)) {
    std::cout << "unknown quirk \"" << unknown << "\", expected none, vip, schip, octo, shift, load-store, vf-reset, jump, wrap or vblank" << std::endl;
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

static void usage(const char *program) {
  std::cout << "Usage : " << program << " [--display tui|window] [--quirks LIST] [--speed N] ROMNAME\n\n"
            << "  --display tui     draw in this terminal" << (DEFAULT_MODE == DisplayMode::Terminal ? " (default)" : "") << "\n"
            << "  --display window  open a separate window" << (DEFAULT_MODE == DisplayMode::Window ? " (default)" : "") << "\n"
            << "  --quirks LIST     comma separated presets (none, vip, schip, octo) and quirks\n"
            << "                    (shift, load-store, vf-reset, jump, wrap, vblank) to turn on,\n"
            << "                    starting from the default\n"
            << "  --speed N         opcodes run per frame\n\n"
            << "Without --quirks or --speed, they are read from the ROM's .cfg file if it has one,\n"
            << "otherwise they default to " << DEFAULT_QUIRKS << " and " << DEFAULT_SPEED << "." << std::endl;
}

int main(int argc, char *argv[]) {
  DisplayMode mode = DEFAULT_MODE;
  // set from the command line, otherwise from the ROM's settings file
  RomSettings options;
  const char *romPath = nullptr;
  for(int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if(arg.rfind("--display=", 0) == 0) {
      if(!parseMode(arg.substr(10), mode)) return 1;
    } else if(arg == "--display") {
      if(++i == argc) { std::cout << "--display needs a value" << std::endl; return 1; }
      if(!parseMode(argv[i], mode)) return 1;
    } else if(arg.rfind("--quirks=", 0) == 0) {
      if(!parseQuirks(arg.substr(9), options.quirks.emplace(defaultQuirks()))) return 1;
    } else if(arg == "--quirks") {
      if(++i == argc) { std::cout << "--quirks needs a value" << std::endl; return 1; }
      if(!parseQuirks(argv[i], options.quirks.emplace(defaultQuirks()))) return 1;
    } else if(arg.rfind("--speed=", 0) == 0) {
      if(!parseSpeedOption(arg.substr(8), options.speed.emplace())) return 1;
    } else if(arg == "--speed") {
      if(++i == argc) { std::cout << "--speed needs a value" << std::endl; return 1; }
      if(!parseSpeedOption(argv[i], options.speed.emplace())) return 1;
    } else if(arg == "-h" || arg == "--help") {
      usage(argv[0]);
      return 0;
    } else if(!romPath) {
      romPath = argv[i];
    } else {
      std::cout << "unexpected argument \"" << arg << "\"\n\n";
      usage(argv[0]);
      return 1;
    }
  }
  if(!romPath) {
    usage(argv[0]);
    return 0;
  }

  std::vector<unsigned char> rom;
  if(!readRom(romPath, rom)) return 1;

  RomSettings romSettings;
  std::string error;
  if(!loadRomSettings(romPath, romSettings, error)) {
    std::cout << error << std::endl;
    return 1;
  }
  Quirks quirks = options.quirks.value_or(romSettings.quirks.value_or(defaultQuirks()));
  int speed = options.speed.value_or(romSettings.speed.value_or(DEFAULT_SPEED));

  /* Initialize defaults, and Video for the window; Sound opens the audio device itself */
  if(!SDL_Init(mode == DisplayMode::Window ? SDL_INIT_VIDEO : 0)) {
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  int status = 0;
  try {
    // scoped so the screen and audio device close before SDL_Quit
    Emulator emulator(static_cast<std::string>("Chip 8 : ") + romPath, mode);
    emulator.setQuirks(quirks);
    emulator.setSpeed(speed);
    emulator.load(rom.data(), rom.size());

    while(emulator.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      emulator.tick();
    }
  } catch(const std::runtime_error &e) {
    // reported once the terminal is restored
    std::cout << e.what() << std::endl;
    status = -1;
  }

  SDL_Quit();

  return status;
}
