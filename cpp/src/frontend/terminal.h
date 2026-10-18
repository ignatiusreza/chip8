#ifndef _CHIP8_FRONTEND_TERMINAL_H
#define	_CHIP8_FRONTEND_TERMINAL_H

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include "layout.h"
#include "screen.h"
#include "../core/settings.h"

struct termios;

// The terminal the emulator was started from, showing the display and the ROM's how to
// play, centred, and reading the keyboard. Only available on POSIX systems.
class Terminal : public Screen {
  public:
    using Clock = std::chrono::steady_clock;

  private:
    std::string _title;
    RomSettings _howTo;
    Layout _layout;
    // the terminal settings to restore
    std::unique_ptr<termios> _saved;
    std::array<Clock::time_point, 16> _lastPress;
    bool _quit = false;
    int _width = 0, _height = 0;

  public:
    // switches the terminal to raw mode and an alternate screen, which are restored
    // when the Terminal is destroyed; throws std::runtime_error if stdin or stdout
    // isn't a terminal
    // `howTo` is the ROM's settings, whose how to play is shown next to the game
    Terminal(const std::string &title, const RomSettings &howTo);
    ~Terminal() override;

    Terminal(const Terminal &) = delete;
    Terminal &operator=(const Terminal &) = delete;

    Keys keys() override;

    // redraws if the display changed or the terminal was resized, which lays the game and
    // how to play out again
    void draw(Display &display) override;

  private:
    void handle(const unsigned char *buf, int length);
};

#endif	/* _CHIP8_FRONTEND_TERMINAL_H */
