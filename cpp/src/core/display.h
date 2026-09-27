#ifndef _CHIP8_CORE_DISPLAY_H
#define	_CHIP8_CORE_DISPLAY_H

#include <bitset>

// Monochrome 64x32 frame buffer.
class Display {
  public:
    static constexpr int WIDTH = 64;
    static constexpr int HEIGHT = 32;
    static constexpr int BUFF_LENGTH = WIDTH * HEIGHT;

  private:
    std::bitset<BUFF_LENGTH> _buff;
    bool _is_invalidated;

  public:
    Display() : _is_invalidated(true) {}

    void clearScreen() { _buff.reset(); _is_invalidated = true; }
    // off-screen pixels read as unset, instead of wrapping to the next row
    bool get(int x, int y) const {
      if(!onScreen(x, y)) return false;
      return _buff.test(x + (y*WIDTH));
    }
    // off-screen pixels are ignored
    void flip(int x, int y) {
      if(!onScreen(x, y)) return;
      _buff.flip(x + (y*WIDTH));
      _is_invalidated = true;
    }

    // returns true once after the buffer has changed, so the frontend only redraws when needed
    bool takeInvalidated() {
      bool invalidated = _is_invalidated;
      _is_invalidated = false;
      return invalidated;
    }

  private:
    static bool onScreen(int x, int y) { return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT; }
};

#endif	/* _CHIP8_CORE_DISPLAY_H */
