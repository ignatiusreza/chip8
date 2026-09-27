#include "graphic.h"

#include <stdexcept>

Graphic::Graphic(const std::string &title)
    : _window(nullptr, SDL_DestroyWindow), _renderer(nullptr, SDL_DestroyRenderer) {
  SDL_Window *window;
  SDL_Renderer *renderer;
  if(!SDL_CreateWindowAndRenderer(title.c_str(), Display::WIDTH*SCALE, Display::HEIGHT*SCALE, 0,
                                  &window, &renderer)) {
    throw std::runtime_error(std::string("Could not open the window: ") + SDL_GetError());
  }
  _window.reset(window);
  _renderer.reset(renderer);
}

void Graphic::update(Display &display, bool force) {
  if(!display.takeInvalidated() && !force) return;

  SDL_Renderer *renderer = _renderer.get();
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  for(int j = 0;j < Display::HEIGHT;j++) {
    for(int i = 0;i < Display::WIDTH;i++) {
      if(display.get(i, j)) {
        SDL_FRect rect = {float(i*SCALE), float(j*SCALE), float(SCALE), float(SCALE)};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  /* Update the screen */
  SDL_RenderPresent(renderer);
}
