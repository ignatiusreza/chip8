#ifndef _CHIP8_FRONTEND_GRAPHIC_H
#define	_CHIP8_FRONTEND_GRAPHIC_H

#include "SDL.h"
#include "../core/display.h"

// The emulator window, showing the 64x32 display scaled 10x.
class Graphic {
  public:
    static const int SCALE = 10;

  private:
    SDL_Surface *_screen;
    Uint32 _black, _white;
    SDL_Rect _rect;

  public:
    Graphic();

    // redraws the window if the display changed
    void update(Display &display);
};

#endif	/* _CHIP8_FRONTEND_GRAPHIC_H */
