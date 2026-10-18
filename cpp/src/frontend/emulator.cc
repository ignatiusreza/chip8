#include "emulator.h"

#include <chrono>
#include <thread>

#include "../core/settings.h"
#include "terminal.h"
#include "window.h"

static std::unique_ptr<Screen> openScreen(const std::string &title, DisplayMode mode,
                                          const RomSettings &howTo) {
  if(mode == DisplayMode::Window) return std::make_unique<Window>(title);
  return std::make_unique<Terminal>(title, howTo);
}

Emulator::Emulator(const Options &options)
    : _screen(openScreen("Chip 8 : " + options.romPath, options.mode, options.settings)),
      _speed(options.speed) {
  _cpu.setQuirks(options.quirks);
  // parseOptions already checked that it fits
  _cpu.load(options.rom.data(), options.rom.size());
}

void Emulator::run() {
  while(!_screen->quitRequested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    tick();
  }
}

void Emulator::tick() {
  // play audio for as long as the sound timer runs
  _sound.setPlaying(_cpu.tickTimers());

  for(int i = 0;i < _speed;i++) {
    // check for keyboard state
    _screen->poll(_cpu);
    _cpu.step();
  }

  // update screen if invalidated
  _screen->draw(_cpu.display());
}
