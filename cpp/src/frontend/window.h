#ifndef _CHIP8_FRONTEND_WINDOW_H
#define	_CHIP8_FRONTEND_WINDOW_H

#include <string>
#include "graphic.h"
#include "input.h"
#include "screen.h"

// A desktop window, drawn and read through SDL. SDL video must be initialized first.
class Window : public Screen {
  Graphic _graphic;
  Input   _input;

  public:
    // opens the window; throws std::runtime_error if it can't
    explicit Window(const std::string &title) : _graphic(title) {}

    void poll(CPU &cpu) override { _input.poll(cpu); }
    bool quitRequested() const override { return _input.quitRequested(); }

    // redraws if the display changed, or the window was uncovered
    void draw(Display &display) override { _graphic.update(display, _input.takeExposed()); }
};

#endif	/* _CHIP8_FRONTEND_WINDOW_H */
