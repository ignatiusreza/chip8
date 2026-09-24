#ifndef _CHIP8_GRAPHIC_H
#define	_CHIP8_GRAPHIC_H

#include <bitset>
#include "SDL.h"

class Graphic {
  public:
    static const int WIDTH = 64;
    static const int HEIGHT = 32;
    static const int BUFF_LENGTH = WIDTH * HEIGHT;

  private:
    std::bitset<BUFF_LENGTH> _buff;
    bool _is_invalidated;

    SDL_Surface *_screen;
    Uint32 _black, _white;
    SDL_Rect _rect;

  public:
    Graphic() {
      _screen = SDL_SetVideoMode(WIDTH*10, HEIGHT*10, 0, SDL_ANYFORMAT);
      
      _black = SDL_MapRGB(_screen->format, 0x00, 0x00, 0x00);
      _white = SDL_MapRGB(_screen->format, 0xff, 0xff, 0xff);
      _rect.h = _rect.w = 10;
    }
    void clearScreen() { _buff.reset(); _is_invalidated = true; }
    bool get (int x,int y) { 
      try {
        return _buff.test(x + (y*WIDTH));
      } catch(...) {
        return false;
      }
    }
    void flip(int x,int y) { 
      try {
        _buff.flip(x + (y*WIDTH));
        _is_invalidated = true;
      } catch(...) { }
    }

    void invalidate() { _is_invalidated = true; }

    void updateScreen() {
      if(!_is_invalidated) return;

      SDL_FillRect(_screen, NULL, _black);
      for(int j = 0;j < HEIGHT;j++) {
        for(int i = 0;i < WIDTH;i++) {
          if(_buff.test(i + (j*WIDTH))) {
            _rect.x = i*10;
            _rect.y = j*10;
            SDL_FillRect(_screen, &_rect, _white);
          }
        }
      }

      /* Update the screen */
      SDL_UpdateRect(_screen, 0, 0, 0, 0);

      _is_invalidated = false;
    }
};

#endif	/* _CHIP8_GRAPHIC_H */
