#ifndef _CHIP8_FRONTEND_TERMINAL_H
#define	_CHIP8_FRONTEND_TERMINAL_H

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include "screen.h"

struct termios;

// The terminal the emulator was started from, showing the 64x32 display as 64x16
// half-block characters and reading the keyboard. Only available on POSIX systems.
class Terminal : public Screen {
  public:
    using Clock = std::chrono::steady_clock;

  private:
    std::string _title;
    // the terminal settings to restore
    std::unique_ptr<termios> _saved;
    std::array<Clock::time_point, 16> _lastPress;
    std::array<bool, 16> _down{};
    bool _quit = false;
    int _width = 0, _height = 0;

  public:
    // switches the terminal to raw mode and an alternate screen, which are restored
    // when the Terminal is destroyed; throws std::runtime_error if stdin or stdout
    // isn't a terminal
    explicit Terminal(const std::string &title);
    ~Terminal() override;

    Terminal(const Terminal &) = delete;
    Terminal &operator=(const Terminal &) = delete;

    void poll(CPU &cpu) override;
    bool quitRequested() const override { return _quit; }

    // redraws if the display changed or the terminal was resized
    void draw(Display &display) override;

  private:
    void handle(const unsigned char *buf, int length);
};

#endif	/* _CHIP8_FRONTEND_TERMINAL_H */
