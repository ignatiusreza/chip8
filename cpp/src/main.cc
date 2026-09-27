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

static void usage(const char *program) {
  std::cout << "Usage : " << program << " [--display tui|window] ROMNAME\n\n"
            << "  --display tui     draw in this terminal" << (DEFAULT_MODE == DisplayMode::Terminal ? " (default)" : "") << "\n"
            << "  --display window  open a separate window" << (DEFAULT_MODE == DisplayMode::Window ? " (default)" : "") << std::endl;
}

int main(int argc, char *argv[]) {
  DisplayMode mode = DEFAULT_MODE;
  const char *romPath = nullptr;
  for(int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if(arg.rfind("--display=", 0) == 0) {
      if(!parseMode(arg.substr(10), mode)) return 1;
    } else if(arg == "--display") {
      if(++i == argc) { std::cout << "--display needs a value" << std::endl; return 1; }
      if(!parseMode(argv[i], mode)) return 1;
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

  /* Initialize defaults, and Video for the window; Sound opens the audio device itself */
  if(!SDL_Init(mode == DisplayMode::Window ? SDL_INIT_VIDEO : 0)) {
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  int status = 0;
  try {
    // scoped so the screen and audio device close before SDL_Quit
    Emulator emulator(static_cast<std::string>("Chip 8 : ") + romPath, mode);
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
