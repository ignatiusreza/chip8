#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "SDL.h"
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

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cout << "Usage : " << argv[0] << " ROMNAME" << std::endl;
    return 0;
  }

  std::vector<unsigned char> rom;
  if(!readRom(argv[1], rom)) return 1;

  /* Initialize defaults, Video and Audio */
  if((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) {
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  SDL_WM_SetCaption((static_cast<std::string>("Chip 8 : ") +  argv[1]).c_str(),NULL);
  {
    // scoped so the window and audio device close before SDL_Quit
    Emulator emulator;
    emulator.load(rom.data(), rom.size());

    while(emulator.isRunning()) {
      SDL_Delay(16);
      emulator.tick();
    }
  }

  SDL_Quit();

  return 0;
}
