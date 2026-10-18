#ifndef _CHIP8_CHIP8_DISPLAY_H
#define	_CHIP8_CHIP8_DISPLAY_H

#include <bitset>

// Monochrome frame buffer, 64x32 in low resolution or 128x64 in the SUPER-CHIP high
// resolution. The frame is always kept at 128x64, with each low resolution pixel
// covering 2x2 frame pixels, so frontends can draw it the same way in either mode.
class Display {
  public:
    static constexpr int FRAME_WIDTH = 128;
    static constexpr int FRAME_HEIGHT = 64;

  private:
    std::bitset<FRAME_WIDTH * FRAME_HEIGHT> _frame;
    bool _hires;
    bool _is_invalidated;

  public:
    Display() : _hires(false), _is_invalidated(true) {}

    bool hires() const { return _hires; }
    // switches the resolution, clearing the screen
    void setHires(bool hires) { _hires = hires; clearScreen(); }

    // the size in pixels of the current resolution
    int width() const { return FRAME_WIDTH / scale(); }
    int height() const { return FRAME_HEIGHT / scale(); }

    void clearScreen() { _frame.reset(); _is_invalidated = true; }
    // off-screen pixels read as unset, instead of wrapping to the next row
    bool get(int x, int y) const {
      if(!onScreen(x, y)) return false;
      return pixel(x * scale(), y * scale());
    }
    // off-screen pixels are ignored
    void flip(int x, int y) {
      if(!onScreen(x, y)) return;
      for(int dy = 0; dy < scale(); dy++)
        for(int dx = 0; dx < scale(); dx++)
          _frame.flip(x * scale() + dx + (y * scale() + dy) * FRAME_WIDTH);
      _is_invalidated = true;
    }

    // scrolls the screen by n pixels of the current resolution, the pixels scrolled in
    // are unset
    void scrollDown(int n) { scroll(0, n); }
    void scrollLeft(int n) { scroll(-n, 0); }
    void scrollRight(int n) { scroll(n, 0); }

    // a pixel of the 128x64 frame, for frontends
    bool pixel(int x, int y) const { return _frame.test(x + y * FRAME_WIDTH); }

    // returns true once after the buffer has changed, so the frontend only redraws when needed
    bool takeInvalidated() {
      bool invalidated = _is_invalidated;
      _is_invalidated = false;
      return invalidated;
    }

  private:
    int scale() const { return _hires ? 1 : 2; }
    bool onScreen(int x, int y) const { return x >= 0 && x < width() && y >= 0 && y < height(); }

    void scroll(int dx, int dy) {
      dx *= scale();
      dy *= scale();
      std::bitset<FRAME_WIDTH * FRAME_HEIGHT> scrolled;
      for(int y = 0; y < FRAME_HEIGHT; y++) {
        for(int x = 0; x < FRAME_WIDTH; x++) {
          int fromX = x - dx, fromY = y - dy;
          if(fromX >= 0 && fromX < FRAME_WIDTH && fromY >= 0 && fromY < FRAME_HEIGHT)
            scrolled[x + y * FRAME_WIDTH] = pixel(fromX, fromY);
        }
      }
      _frame = scrolled;
      _is_invalidated = true;
    }
};

#endif	/* _CHIP8_CHIP8_DISPLAY_H */
