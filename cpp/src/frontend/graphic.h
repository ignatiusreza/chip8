#ifndef _CHIP8_FRONTEND_GRAPHIC_H
#define	_CHIP8_FRONTEND_GRAPHIC_H

#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "../core/display.h"

// The emulator window, showing the 64x32 display scaled 10x.
class Graphic {
  public:
    static constexpr int SCALE = 10;

  private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window;
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer;

  public:
    // opens the window; throws std::runtime_error if it can't
    explicit Graphic(const std::string &title);

    // redraws the window if the display changed, or always when forced (e.g. after
    // the window was uncovered, as its contents aren't kept)
    void update(Display &display, bool force = false);
};

#endif	/* _CHIP8_FRONTEND_GRAPHIC_H */
