#include <iostream>
#include <string>
#include "cpu.h"
#include "SDL.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cout << "Usage : " << argv[0] << " ROMNAME" << std::endl;
    return 0;
  }
    
  /* Initialize defaults, Video and Audio */
  if((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) { 
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  SDL_WM_SetCaption((static_cast<std::string>("Chip 8 : ") +  argv[1]).c_str(),NULL);
  CPU cpu;
  cpu.load(argv[1]);

  while(cpu.loop()) {
    SDL_Delay(16);
    cpu.tick();
  }

  SDL_Quit();

  return 0;
}

