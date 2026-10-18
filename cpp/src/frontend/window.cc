#include "window.h"

#include <SDL3/SDL.h>

// the key for each CHIP-8 keypad value, laid out like the COSMAC VIP keypad on the left
// of a QWERTY keyboard:
//   1 2 3 4        1 2 3 C
//   Q W E R   ->   4 5 6 D
//   A S D F        7 8 9 E
//   Z X C V        A 0 B F
// so W A S D are 5 7 8 9, the up, left, down and right of most games. Keys are matched by
// position, so the layout is the same on non-QWERTY keyboards.
static constexpr SDL_Scancode KEYMAP[16] = {
  SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, // 0 1 2 3
  SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A, // 4 5 6 7
  SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, // 8 9 A B
  SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V, // C D E F
};

Keys Window::keys() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_EVENT_WINDOW_EXPOSED) _exposed = true;
    else if(event.type == SDL_EVENT_QUIT) _quit = true;
  }

  const bool *state = SDL_GetKeyboardState(nullptr);
  Keys keys;
  for(int k = 0; k < 16; k++) keys.down[k] = state[KEYMAP[k]];
  keys.quit = _quit || state[SDL_SCANCODE_ESCAPE];
  return keys;
}

void Window::draw(Display &display) {
  _graphic.update(display, _exposed);
  _exposed = false;
}
