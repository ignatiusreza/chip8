#ifndef _CHIP8_FRONTEND_WINDOW_H
#define	_CHIP8_FRONTEND_WINDOW_H

#include <string>
#include "graphic.h"
#include "screen.h"

// A desktop window, drawn and read through SDL. SDL video must be initialized first.
class Window : public Screen {
  Graphic _graphic;
  bool _quit = false;
  // set when the window was uncovered and needs redrawing
  bool _exposed = false;

  public:
    // opens the window; throws std::runtime_error if it can't
    explicit Window(const std::string &title) : _graphic(title) {}

    Keys keys() override;

    // redraws if the display changed, or the window was uncovered
    void draw(Display &display) override;
};

#endif	/* _CHIP8_FRONTEND_WINDOW_H */
