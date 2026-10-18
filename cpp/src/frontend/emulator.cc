#include "emulator.h"

#include "settings.h"
#include "terminal.h"
#include "window.h"

static std::unique_ptr<Screen> openScreen(const std::string &title, DisplayMode mode,
                                          const RomSettings &howTo) {
  if(mode == DisplayMode::Window) return std::make_unique<Window>(title);
  return std::make_unique<Terminal>(title, howTo);
}

Emulator::Emulator(const std::string &title, DisplayMode mode, const RomSettings &howTo)
    : _screen(openScreen(title, mode, howTo)), _speed(DEFAULT_SPEED) {}

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
