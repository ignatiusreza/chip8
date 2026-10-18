#include "terminal.h"

#include <cstdio>
#include <stdexcept>

#ifdef _WIN32

struct termios {};

Terminal::Terminal(const std::string &, const RomSettings &) {
  throw std::runtime_error("the terminal display isn't supported on Windows, use --display window");
}
Terminal::~Terminal() = default;
void Terminal::poll(CPU &) {}
void Terminal::draw(Display &) {}
void Terminal::handle(const unsigned char *, int) {}

#else

#include <cctype>
#include <cstring>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// Terminals only send key presses, repeated while the key is held. Without release
// events, a key counts as held for this long after its last press, which bridges the gap
// before the terminal starts repeating it.
static constexpr auto HOLD = std::chrono::milliseconds(200);

// character for each CHIP-8 keypad value, laid out like the COSMAC
// VIP keypad on the left of a QWERTY keyboard:
//   1 2 3 4        1 2 3 C
//   Q W E R   ->   4 5 6 D
//   A S D F        7 8 9 E
//   Z X C V        A 0 B F
// so W A S D are 5 7 8 9, the up, left, down and right of most games
static constexpr char KEYMAP[] = "x123qweasdzc4rfv";

// the half block showing two pixels, one above the other
static const char *halfBlock(bool top, bool bottom) {
  return top && bottom ? "█" : top ? "▀" : bottom ? "▄" : " ";
}

// moves the cursor to the 0-based column x and row y
static std::string moveTo(int x, int y) {
  return "\x1b[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) + "H";
}

// the braille character showing the 2x4 high resolution pixels at (x, y)
static std::string braille(const Display &display, int x, int y) {
  // the bit of each dot, by column then row
  static const int DOTS[2][4] = {{0x01, 0x02, 0x04, 0x40}, {0x08, 0x10, 0x20, 0x80}};
  int bits = 0;
  for(int dx = 0; dx < 2; dx++)
    for(int dy = 0; dy < 4; dy++)
      if(display.pixel(x + dx, y + dy)) bits |= DOTS[dx][dy];
  // U+2800 + bits, in UTF-8
  return {char(0xE2), char(0xA0 | (bits >> 6)), char(0x80 | (bits & 0x3F))};
}

static void writeOut(const std::string &s) {
  for(std::size_t done = 0; done < s.size();) {
    ssize_t n = write(STDOUT_FILENO, s.data() + done, s.size() - done);
    if(n <= 0) return;
    done += n;
  }
}

Terminal::Terminal(const std::string &title, const RomSettings &howTo) : _title(title), _howTo(howTo) {
  if(!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
    throw std::runtime_error("the terminal display needs an interactive terminal, try --display window");
  }

  termios raw;
  if(tcgetattr(STDIN_FILENO, &raw) != 0) throw std::runtime_error("Could not read the terminal settings");
  _saved = std::make_unique<termios>(raw);

  // no echo, line buffering or signals; reads return at once, with whatever was typed
  cfmakeraw(&raw);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  // alternate screen, hidden cursor, window title
  writeOut("\x1b[?1049h\x1b[?25l\x1b]0;" + title + "\a");
}

Terminal::~Terminal() {
  writeOut("\x1b[?25h\x1b[?1049l");
  tcsetattr(STDIN_FILENO, TCSAFLUSH, _saved.get());
}

void Terminal::poll(CPU &cpu) {
  unsigned char buf[64];
  int length;
  while((length = read(STDIN_FILENO, buf, sizeof(buf))) > 0) handle(buf, length);

  // only report changes, so FX0A waits for a fresh keypress
  Clock::time_point now = Clock::now();
  for(int k = 0; k < 16; k++) {
    bool down = _lastPress[k] != Clock::time_point() && now - _lastPress[k] < HOLD;
    if(down != _down[k]) {
      _down[k] = down;
      cpu.setKey(k, down);
    }
  }
}

void Terminal::handle(const unsigned char *buf, int length) {
  for(int i = 0; i < length; i++) {
    unsigned char c = buf[i];
    if(c == 0x03) { // Ctrl-C
      _quit = true;
    } else if(c == 0x1b && i == length - 1) { // a lone Esc
      _quit = true;
    } else if(c == 0x1b && (buf[i+1] == '[' || buf[i+1] == 'O')) {
      // skip escape sequences, such as arrow keys, up to their final byte
      for(i += 2; i < length && (buf[i] < 0x40 || buf[i] > 0x7e); i++) {}
    } else if(const char *key = c ? std::strchr(KEYMAP, std::tolower(c)) : nullptr) {
      _lastPress[key - KEYMAP] = Clock::now();
    }
  }
}

void Terminal::draw(Display &display) {
  winsize size{};
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
  bool resized = size.ws_col != _width || size.ws_row != _height;
  _width = size.ws_col;
  _height = size.ws_row;
  if(!display.takeInvalidated() && !resized) return;

  // the title, border and how to play are only drawn when resized, which clears the screen
  std::string out;
  const Layout &l = _layout;
  int cellsWide = gameColumns(l.scale) - 2, cellsHigh = gameRows(l.scale) - 3;
  if(resized) {
    _layout = layoutTerminal(_width, _height, _howTo);
    cellsWide = gameColumns(l.scale) - 2;
    cellsHigh = gameRows(l.scale) - 3;
    out += "\x1b[2J";
    if(!l.fits) {
      out += "\x1b[HTerminal too small, needs " + std::to_string(gameColumns(1)) + "x" + std::to_string(gameRows(1));
      writeOut(out);
      return;
    }

    std::string title = prefix(_title + " (Esc to quit)", cellsWide + 2), border;
    for(int x = 0; x < cellsWide; x++) border += "─";
    out += moveTo(l.gameX, l.gameY) + title;
    out += moveTo(l.gameX, l.gameY + 1) + "┌" + border + "┐";
    for(int row = 0; row < cellsHigh; row++) {
      out += moveTo(l.gameX, l.gameY + 2 + row) + "│";
      out += moveTo(l.gameX + cellsWide + 1, l.gameY + 2 + row) + "│";
    }
    out += moveTo(l.gameX, l.gameY + cellsHigh + 2) + "└" + border + "┘";
    for(std::size_t i = 0; i < l.panel.size(); i++) out += moveTo(l.panelX, l.panelY + i) + l.panel[i];
  } else if(!l.fits) {
    return;
  }

  for(int row = 0; row < cellsHigh; row++) {
    out += moveTo(l.gameX + 1, l.gameY + 2 + row);
    for(int col = 0; col < cellsWide; col++) {
      if(l.scale == 2) {
        // two frame pixels, one above the other
        out += halfBlock(display.pixel(col, row * 2), display.pixel(col, row * 2 + 1));
      } else if(display.hires()) {
        out += braille(display, col * 2, row * 4);
      } else {
        // two low resolution pixels, one above the other
        out += halfBlock(display.get(col, row * 2), display.get(col, row * 2 + 1));
      }
    }
  }
  writeOut(out);
}

#endif
