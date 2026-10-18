#include "layout.h"

#include <algorithm>
#include <sstream>
#include "../core/display.h"

// the gap between the game and how to play beside it
static constexpr int PANEL_GAP = 3;
// how narrow and wide how to play beside the game can be
static constexpr int PANEL_MIN_WIDTH = 24;
static constexpr int PANEL_MAX_WIDTH = 44;

int gameColumns(int scale) { return Display::FRAME_WIDTH * scale / 2 + 2; }
int gameRows(int scale) { return Display::FRAME_HEIGHT * scale / 4 + 3; }

int textWidth(const std::string &text) {
  // count the UTF-8 characters, not their continuation bytes
  return std::count_if(text.begin(), text.end(), [](char c) { return (c & 0xC0) != 0x80; });
}

std::string prefix(const std::string &text, int width) {
  std::size_t end = 0;
  for(int n = 0; end < text.size(); end++) {
    if((text[end] & 0xC0) != 0x80 && n++ == width) break;
  }
  return text.substr(0, end);
}

std::vector<std::string> wrap(const std::string &text, int width, int indent) {
  std::vector<std::string> lines;
  std::istringstream words(text);
  std::string word, line;
  while(words >> word) {
    if(line.empty()) line = word;
    else if(textWidth(line) + 1 + textWidth(word) <= width) line += " " + word;
    else {
      lines.push_back(line);
      line = std::string(indent, ' ') + word;
    }
    while(textWidth(line) > width) {
      std::string head = prefix(line, width);
      lines.push_back(head);
      line = std::string(indent, ' ') + line.substr(head.size());
    }
  }
  if(!line.empty()) lines.push_back(line);
  return lines;
}

std::vector<std::string> howToPlay(const RomSettings &settings, int width) {
  std::vector<std::string> lines;
  auto section = [&](const std::vector<std::string> &texts, int indent) {
    if(texts.empty()) return;
    if(!lines.empty()) lines.push_back("");
    for(const std::string &text : texts) {
      std::vector<std::string> wrapped = wrap(text, width, indent);
      lines.insert(lines.end(), wrapped.begin(), wrapped.end());
    }
  };
  std::vector<std::string> heading;
  if(!settings.title.empty()) heading.push_back(settings.title);
  if(!settings.author.empty()) heading.push_back("by " + settings.author);
  section(heading, 0);
  section(settings.about, 0);
  if(!settings.controls.empty()) {
    std::vector<std::string> controls{"Controls"};
    controls.insert(controls.end(), settings.controls.begin(), settings.controls.end());
    section(controls, 2);
  }
  std::vector<std::string> running;
  if(settings.quirks) running.push_back("Quirks: " + settings.quirks->describe());
  if(settings.speed) running.push_back("Speed: " + std::to_string(*settings.speed) + " opcodes per frame");
  section(running, 2);
  return lines;
}

static std::vector<std::string> truncate(std::vector<std::string> lines, int n) {
  if(int(lines.size()) > n) lines.resize(n);
  return lines;
}

static Layout besides(int columns, int rows, int scale, std::vector<std::string> panel) {
  int gw = gameColumns(scale), gh = gameRows(scale), panelWidth = 0;
  for(const std::string &line : panel) panelWidth = std::max(panelWidth, textWidth(line));
  int width = gw + PANEL_GAP + panelWidth, height = std::max(gh, int(panel.size()));
  int x = (columns - width) / 2, y = (rows - height) / 2;
  int panelY = y + (height - int(panel.size())) / 2;
  return {true, scale, x, y + (height - gh) / 2, x + gw + PANEL_GAP, panelY, std::move(panel)};
}

static Layout below(int columns, int rows, int scale, std::vector<std::string> panel) {
  int gw = gameColumns(scale), gh = gameRows(scale);
  int height = gh + (panel.empty() ? 0 : 1 + int(panel.size()));
  int x = (columns - gw) / 2, y = (rows - height) / 2;
  // how to play lines up with the inside of the game's border
  return {true, scale, x, y, x + 1, y + gh + 1, std::move(panel)};
}

Layout layoutTerminal(int columns, int rows, const RomSettings &settings) {
  bool hasPanel = !howToPlay(settings, PANEL_MAX_WIDTH).empty();
  // characters are about twice as tall as they are wide
  bool wide = columns > rows * 2;
  for(int scale : {2, 1}) {
    if(!hasPanel) break;
    int gw = gameColumns(scale), gh = gameRows(scale);
    bool besidesFits = columns >= gw + PANEL_GAP + PANEL_MIN_WIDTH && rows >= gh;
    std::vector<std::string> belowPanel = howToPlay(settings, gw - 2);
    bool belowFits = columns >= gw && rows >= gh + 1 + int(belowPanel.size());
    if(besidesFits && (wide || !belowFits)) {
      int width = std::min(PANEL_MAX_WIDTH, columns - gw - PANEL_GAP);
      return besides(columns, rows, scale, truncate(howToPlay(settings, width), rows));
    }
    if(belowFits) return below(columns, rows, scale, belowPanel);
  }
  // below the normal size game, as much of how to play as fits
  int gh = gameRows(1);
  if(hasPanel && columns >= gameColumns(1) && rows >= gh + 2) {
    return below(columns, rows, 1, truncate(howToPlay(settings, gameColumns(1) - 2), rows - gh - 1));
  }
  for(int scale : {2, 1}) {
    if(columns >= gameColumns(scale) && rows >= gameRows(scale)) return below(columns, rows, scale, {});
  }
  return {};
}
