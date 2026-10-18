#include "emulator.h"

#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

#include "../chip8/settings.h"
#include "terminal.h"
#include "window.h"

// reads the whole ROM file; throws std::runtime_error if it can't
static std::vector<unsigned char> readRom(const std::string &path) {
  std::ifstream file(path, std::ios_base::binary | std::ios_base::in);
  if(!file) throw std::runtime_error("Could not read " + path);
  std::vector<unsigned char> rom(std::istreambuf_iterator<char>(file), {});
  if(file.bad()) throw std::runtime_error("Error Reading ROM");
  return rom;
}

static std::unique_ptr<Screen> openScreen(const std::string &title, DisplayMode mode,
                                          const RomSettings &howTo) {
  if(mode == DisplayMode::Window) return std::make_unique<Window>(title);
  return std::make_unique<Terminal>(title, howTo);
}

Emulator::Emulator(const Args &args) {
  std::vector<unsigned char> rom = readRom(args.romPath);
  // the command line wins over the ROM's settings file, which wins over the defaults
  RomSettings howTo;
  std::string error;
  if(!loadRomSettings(args.romPath, howTo, error)) throw std::runtime_error(error);
  Quirks quirks = args.quirks.value_or(howTo.quirks.value_or(defaultQuirks()));
  _speed = args.speed.value_or(howTo.speed.value_or(DEFAULT_SPEED));
  // the terminal shows the quirks and speed the ROM runs with, alongside its how to play
  howTo.quirks = quirks;
  howTo.speed = _speed;

  _cpu.setQuirks(quirks);
  if(!_cpu.load(rom.data(), rom.size())) {
    throw std::runtime_error("ROM is too large, at most " + std::to_string(CPU::MAX_ROM_SIZE) + " bytes fit in memory");
  }
  _sound.emplace();
  _screen = openScreen("Chip 8 : " + args.romPath, args.mode, howTo);
}

void Emulator::run() {
  while(!_input.quitRequested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    tick();
  }
}

void Emulator::tick() {
  // play audio for as long as the sound timer runs
  _sound->setPlaying(_cpu.tickTimers());
  _input.update(_screen->keys(), _cpu);

  for(int i = 0; i < _speed; i++) _cpu.step();

  // update screen if invalidated
  _screen->draw(_cpu.display());
}
