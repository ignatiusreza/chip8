#include <iostream>
#include <stdexcept>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "frontend/emulator.h"
#include "frontend/options.h"

int main(int argc, char *argv[]) {
  Options options;
  int status;
  if(!parseOptions(argc, argv, options, status)) return status;

  /* Initialize defaults, and Video for the window; Sound opens the audio device itself */
  if(!SDL_Init(options.mode == DisplayMode::Window ? SDL_INIT_VIDEO : 0)) {
    std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;

    return -1;
  }

  try {
    // scoped so the screen and audio device close before SDL_Quit
    Emulator(options).run();
  } catch(const std::runtime_error &e) {
    // reported once the terminal is restored
    std::cout << e.what() << std::endl;
    status = -1;
  }

  SDL_Quit();

  return status;
}
