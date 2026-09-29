#include "terminal.h"

#include <cstdio>
#include <stdexcept>

#ifdef _WIN32

struct termios {};

Terminal::Terminal(const std::string &) {
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

// character for each CHIP-8 keypad value, on the left side of a QWERTY keyboard
static constexpr char KEYMAP[] = "1qweasdzxcrfvtgb";

// the display plus a one character border, and a title line above it
static constexpr int COLUMNS = Display::WIDTH + 2;
static constexpr int ROWS = Display::HEIGHT / 2 + 3;

static void writeOut(const std::string &s) {
  for(std::size_t done = 0; done < s.size();) {
    ssize_t n = write(STDOUT_FILENO, s.data() + done, s.size() - done);
    if(n <= 0) return;
    done += n;
  }
}

Terminal::Terminal(const std::string &title) : _title(title) {
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

  // each frame overwrites the last, so the terminal only needs clearing when resized
  std::string out = resized ? "\x1b[2J\x1b[H" : "\x1b[H";
  if(_width < COLUMNS || _height < ROWS) {
    out += "Terminal too small, needs " + std::to_string(COLUMNS) + "x" + std::to_string(ROWS);
    writeOut(out);
    return;
  }

  std::string border;
  for(int x = 0; x < Display::WIDTH; x++) border += "─";

  out += _title + " (Esc to quit)\r\n";
  out += "┌" + border + "┐\r\n";
  // each character cell holds two pixels, one above the other
  for(int y = 0; y < Display::HEIGHT; y += 2) {
    out += "│";
    for(int x = 0; x < Display::WIDTH; x++) {
      bool top = display.get(x, y), bottom = display.get(x, y + 1);
      out += top && bottom ? "█" : top ? "▀" : bottom ? "▄" : " ";
    }
    out += "│\r\n";
  }
  out += "└" + border + "┘";
  writeOut(out);
}

#endif
