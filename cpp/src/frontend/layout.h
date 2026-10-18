#ifndef _CHIP8_FRONTEND_LAYOUT_H
#define	_CHIP8_FRONTEND_LAYOUT_H

#include <string>
#include <vector>
#include "settings.h"

// The terminal shows the game as a title line above a bordered box of character cells,
// with the ROM's how to play next to it or below it, all centred in the terminal.

// the terminal size the game takes at a scale: 1 for 64x16 cells of 2x4 frame pixels, or
// 2 for 128x32 cells of 1x2 frame pixels
int gameColumns(int scale);
int gameRows(int scale);

// where the game and its how to play go in the terminal; positions are 0-based
struct Layout {
  bool fits = false; // false if the terminal is too small for the game
  int scale = 1;
  int gameX = 0, gameY = 0; // the game's title line
  int panelX = 0, panelY = 0;
  std::vector<std::string> panel; // how to play, wrapped to fit
};

// places the game, as large as possible while how to play still fits next to or below it,
// and centres them in a columns x rows terminal. How to play goes beside the game in a
// terminal wider than it is tall, and below it otherwise, unless only the other fits.
Layout layoutTerminal(int columns, int rows, const RomSettings &settings);

// the ROM's title, author, about, controls, and the quirks and speed it runs with, as
// lines of at most `width` characters, with blank lines between them
std::vector<std::string> howToPlay(const RomSettings &settings, int width);

// breaks text into lines of at most `width` characters at spaces, indenting the lines
// after the first, and splitting words that are too long
std::vector<std::string> wrap(const std::string &text, int width, int indent);

// the width of a line in characters
int textWidth(const std::string &text);

// the first `width` characters of text
std::string prefix(const std::string &text, int width);

#endif	/* _CHIP8_FRONTEND_LAYOUT_H */
