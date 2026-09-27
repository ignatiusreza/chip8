#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
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

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cout << "Usage : " << argv[0] << " ROMNAME" << std::endl;
    return 0;
  }

  std::vector<unsigned char> rom;
  if(!readRom(argv[1], rom)) return 1;

  /* Initialize defaults and Video; Sound opens the audio device itself */
  if(!SDL_Init(SDL_INIT_VIDEO)) {
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  int status = 0;
  try {
    // scoped so the window and audio device close before SDL_Quit
    Emulator emulator(static_cast<std::string>("Chip 8 : ") + argv[1]);
    emulator.load(rom.data(), rom.size());

    while(emulator.isRunning()) {
      SDL_Delay(16);
      emulator.tick();
    }
  } catch(const std::runtime_error &e) {
    std::cout << e.what() << std::endl;
    status = -1;
  }

  SDL_Quit();

  return status;
}
