#include "graphic.h"

Graphic::Graphic() {
  _screen = SDL_SetVideoMode(Display::WIDTH*SCALE, Display::HEIGHT*SCALE, 0, SDL_ANYFORMAT);

  _black = SDL_MapRGB(_screen->format, 0x00, 0x00, 0x00);
  _white = SDL_MapRGB(_screen->format, 0xff, 0xff, 0xff);
  _rect.h = _rect.w = SCALE;
}

void Graphic::update(Display &display) {
  if(!display.takeInvalidated()) return;

  SDL_FillRect(_screen, NULL, _black);
  for(int j = 0;j < Display::HEIGHT;j++) {
    for(int i = 0;i < Display::WIDTH;i++) {
      if(display.get(i, j)) {
        _rect.x = i*SCALE;
        _rect.y = j*SCALE;
        SDL_FillRect(_screen, &_rect, _white);
      }
    }
  }

  /* Update the screen */
  SDL_UpdateRect(_screen, 0, 0, 0, 0);
}
