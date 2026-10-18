#include "emulator.h"

#include <chrono>
#include <stdexcept>
#include <thread>

#include "../chip8/rom.h"
#include "terminal.h"
#include "window.h"

static std::unique_ptr<Screen> openScreen(const std::string &title, DisplayMode mode,
                                          const RomSettings &howTo) {
  if(mode == DisplayMode::Window) return std::make_unique<Window>(title);
  return std::make_unique<Terminal>(title, howTo);
}

Emulator::Emulator(const Args &args) {
  Rom rom;
  std::string error;
  if(!Rom::load(args.romPath, rom, error)) throw std::runtime_error(error);
  // the command line wins over the ROM's settings file, which wins over the defaults
  RomSettings &howTo = rom.settings;
  Quirks quirks = args.quirks.value_or(howTo.quirks.value_or(defaultQuirks()));
  _speed = args.speed.value_or(howTo.speed.value_or(DEFAULT_SPEED));
  // the terminal shows the quirks and speed the ROM runs with, alongside its how to play
  howTo.quirks = quirks;
  howTo.speed = _speed;

  _cpu.setQuirks(quirks);
  if(!_cpu.load(rom.data.data(), rom.data.size())) {
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
