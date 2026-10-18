#ifndef _CHIP8_FRONTEND_WINDOW_H
#define	_CHIP8_FRONTEND_WINDOW_H

#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "screen.h"

// A desktop window, drawn and read through SDL, showing the 128x64 frame scaled 5x, so low
// resolution pixels are scaled 10x. SDL video must be initialized first.
class Window : public Screen {
  public:
    static constexpr int SCALE = 5;

  private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window;
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer;
    bool _quit = false;
    // set when the window was uncovered and needs redrawing, as its contents aren't kept
    bool _exposed = false;

  public:
    // opens the window; throws std::runtime_error if it can't
    explicit Window(const std::string &title);

    Keys keys() override;

    // redraws if the display changed, or the window was uncovered
    void draw(Display &display) override;
};

#endif	/* _CHIP8_FRONTEND_WINDOW_H */
