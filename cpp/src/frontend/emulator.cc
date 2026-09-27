#include "emulator.h"

#include "terminal.h"
#include "window.h"

static const int OPCODES_PER_TICK = 3;

static std::unique_ptr<Screen> openScreen(const std::string &title, DisplayMode mode) {
  if(mode == DisplayMode::Window) return std::make_unique<Window>(title);
  return std::make_unique<Terminal>(title);
}

Emulator::Emulator(const std::string &title, DisplayMode mode) : _screen(openScreen(title, mode)) {}

void Emulator::tick() {
  // play audio for as long as the sound timer runs
  _sound.setPlaying(_cpu.tickTimers());

  for(int i = 0;i < OPCODES_PER_TICK;i++) {
    // check for keyboard state
    _screen->poll(_cpu);
    _cpu.step();
  }

  // update screen if invalidated
  _screen->draw(_cpu.display());
}
