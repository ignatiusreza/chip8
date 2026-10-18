// Terminal layout tests, matching the Go (go/frontend/layout_test.go) and Rust
// (rust/src/frontend/layout.rs) versions.

#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "frontend/layout.h"

namespace {

RomSettings br8kout() {
  RomSettings settings;
  settings.title = "Br8kout";
  settings.author = "SharpenedSpoon";
  settings.about = {"Breakout: bounce the ball off your paddle to break all the bricks."};
  settings.controls = {"A / D: move the paddle left / right"};
  return settings;
}

int panelWidth(const Layout &l) {
  int width = 0;
  for(const std::string &line : l.panel) width = std::max(width, textWidth(line));
  return width;
}

}  // namespace

TEST(LayoutTest, HowToPlay) {
  std::vector<std::string> want{
    "Br8kout",
    "by SharpenedSpoon",
    "",
    "Breakout: bounce the ball off",
    "your paddle to break all the",
    "bricks.",
    "",
    "Controls",
    "A / D: move the paddle left /",
    "  right",
  };
  EXPECT_EQ(howToPlay(br8kout(), 30), want);
  EXPECT_TRUE(howToPlay(RomSettings(), 30).empty());
}

TEST(LayoutTest, HowToPlayShowsQuirksAndSpeed) {
  RomSettings settings = br8kout();
  std::string unknown;
  settings.quirks.emplace().parse("octo,vf-reset", unknown);
  settings.speed = 7;
  std::vector<std::string> lines = howToPlay(settings, 40);
  EXPECT_EQ(std::vector<std::string>(lines.end() - 3, lines.end()),
            (std::vector<std::string>{"", "Quirks: octo,vf-reset", "Speed: 7 opcodes per frame"}));
}

TEST(LayoutTest, WrapSplitsLongWords) {
  EXPECT_EQ(wrap("abcdefgh", 4, 2), (std::vector<std::string>{"abcd", "  ef", "  gh"}));
  EXPECT_EQ(textWidth("█▀ ab"), 5);
}

TEST(LayoutTest, PicksTheLargestGameThatFitsWithHowToPlay) {
  struct Case { int columns, rows, scale; bool besides; };
  for(const Case &c : std::vector<Case>{
        {200, 50, 2, true},  // 130 + 3 + 24 columns fit beside the large game
        {140, 60, 2, false}, // the large game is too wide to fit it beside, but it fits below
        {160, 85, 2, false}, // it fits either way, and the terminal is taller than wide, so below
        {140, 40, 1, true},  // too small for the large game, so the normal one with it beside
        {120, 68, 1, false}, // it fits either way, and the terminal is taller than wide, so below
        {100, 30, 1, true},  // it fits either way, and the terminal is wider than tall, so beside
        {70, 40, 1, false},  // too narrow to put it beside, so below
        {66, 22, 1, false},  // below, cut short
      }) {
    Layout l = layoutTerminal(c.columns, c.rows, br8kout());
    SCOPED_TRACE(std::to_string(c.columns) + "x" + std::to_string(c.rows));
    EXPECT_TRUE(l.fits);
    EXPECT_EQ(l.scale, c.scale);
    EXPECT_EQ(l.panelX > l.gameX + 1, c.besides);
    EXPECT_LE(std::max(l.gameY + gameRows(l.scale), l.panelY + int(l.panel.size())), c.rows);
    EXPECT_LE(std::max(l.gameX + gameColumns(l.scale), l.panelX + panelWidth(l)), c.columns);
  }
}

TEST(LayoutTest, Centres) {
  // the normal size game alone, 66x19, in the middle of 100x29
  Layout l = layoutTerminal(100, 29, RomSettings());
  EXPECT_EQ(l.scale, 1);
  EXPECT_EQ(l.gameX, 17);
  EXPECT_EQ(l.gameY, 5);
  EXPECT_TRUE(l.panel.empty());

  // beside: the game and how to play, 66 + 3 + 29 columns, centred together
  l = layoutTerminal(100, 29, br8kout());
  EXPECT_EQ(l.gameX, 1);
  EXPECT_EQ(l.panelX, 70);
  EXPECT_EQ(l.gameY, 5);
  EXPECT_EQ(l.panelY, 9);
}

TEST(LayoutTest, TooSmall) {
  EXPECT_FALSE(layoutTerminal(65, 40, br8kout()).fits);
  EXPECT_FALSE(layoutTerminal(66, 18, RomSettings()).fits);
}
