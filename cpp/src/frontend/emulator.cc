#include "emulator.h"

static const int OPCODES_PER_TICK = 3;

void Emulator::tick() {
  // play audio for as long as the sound timer runs
  _sound.setPlaying(_cpu.tickTimers());

  for(int i = 0;i < OPCODES_PER_TICK;i++) {
    // check for keyboard state
    _input.poll(_cpu);
    _cpu.step();
  }

  // update screen if invalidated, or uncovered
  _graphic.update(_cpu.display(), _input.takeExposed());
}
