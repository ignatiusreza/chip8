#include "settings.h"

#include <fstream>
#include <sstream>

static std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r");
  if(start == std::string::npos) return "";
  return s.substr(start, s.find_last_not_of(" \t\r") - start + 1);
}

Quirks defaultQuirks() {
  Quirks quirks;
  std::string unknown;
  quirks.parse(DEFAULT_QUIRKS, unknown);
  return quirks;
}

bool parseSpeed(const std::string &value, int &speed) {
  if(value.empty() || value.size() > 5 || value.find_first_not_of("0123456789") != std::string::npos) return false;
  int parsed = std::stoi(value);
  if(parsed < 1 || parsed > MAX_SPEED) return false;
  speed = parsed;
  return true;
}

bool parseRomSettings(const std::string &text, RomSettings &settings, std::string &error) {
  std::istringstream lines(text);
  std::string line;
  for(int number = 1; std::getline(lines, line); number++) {
    line = trim(line);
    if(line.empty() || line[0] == '#') continue;

    std::string where = "line " + std::to_string(number) + ": ";
    size_t equals = line.find('=');
    if(equals == std::string::npos) {
      error = where + "expected \"name = value\"";
      return false;
    }
    std::string name = trim(line.substr(0, equals)), value = trim(line.substr(equals + 1));
    if(name == "quirks") {
      Quirks quirks = defaultQuirks();
      std::string unknown;
      if(!quirks.parse(value, unknown)) {
        error = where + "unknown quirk \"" + unknown + "\"";
        return false;
      }
      settings.quirks = quirks;
    } else if(name == "speed") {
      int speed;
      if(!parseSpeed(value, speed)) {
        error = where + "speed must be a number from 1 to " + std::to_string(MAX_SPEED);
        return false;
      }
      settings.speed = speed;
    } else if(name == "title") {
      settings.title = value;
    } else if(name == "author") {
      settings.author = value;
    } else if(name == "about") {
      settings.about.push_back(value);
    } else if(name == "controls") {
      settings.controls.push_back(value);
    } else {
      error = where + "unknown setting \"" + name + "\", expected quirks, speed, title, author, about or controls";
      return false;
    }
  }
  return true;
}

std::string romSettingsPath(const std::string &romPath) {
  // replace the extension of the file name, not of a directory
  size_t dot = romPath.find_last_of('.');
  size_t slash = romPath.find_last_of("/\\");
  if(dot == std::string::npos || (slash != std::string::npos && dot < slash)) return romPath + ".cfg";
  return romPath.substr(0, dot) + ".cfg";
}

bool loadRomSettings(const std::string &romPath, RomSettings &settings, std::string &error) {
  std::string path = romSettingsPath(romPath);
  std::ifstream file(path);
  if(!file) return true; // the ROM runs with the defaults

  std::stringstream text;
  text << file.rdbuf();
  if(!parseRomSettings(text.str(), settings, error)) {
    error = path + ", " + error;
    return false;
  }
  return true;
}
