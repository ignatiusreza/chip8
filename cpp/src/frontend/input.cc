#include "input.h"

void Input::update(const Keys &keys, CPU &cpu) {
  _quit = keys.quit;

  // only report changes, so FX0A waits for a fresh keypress
  for(int k = 0; k < 16; k++) {
    if(keys.down[k] != _keys[k]) {
      _keys[k] = keys.down[k];
      cpu.setKey(k, keys.down[k]);
    }
  }
}
