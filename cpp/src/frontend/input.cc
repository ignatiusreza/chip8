#include "input.h"

void Input::poll(CPU &cpu) {
  SDL_Event event;
  while( SDL_PollEvent( &event ) ){
    switch( event.type ){
      case SDL_EVENT_KEY_DOWN:
        // ignore auto-repeat, so a held key doesn't count as a fresh press (FX0A)
        if(event.key.repeat) break;
        if(event.key.scancode == SDL_SCANCODE_ESCAPE) _quit = true;
        cpu.setKey(keypadValue(event.key.scancode), true);
        break;
      case SDL_EVENT_KEY_UP:
        cpu.setKey(keypadValue(event.key.scancode), false);
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
        _exposed = true;
        break;
      case SDL_EVENT_QUIT:
        _quit = true;
        break;
    }
  }
}

int Input::keypadValue(SDL_Scancode key) {
  switch( key ){
    case SDL_SCANCODE_1: return 0x0;
    case SDL_SCANCODE_Q: return 0x1;
    case SDL_SCANCODE_W: return 0x2;
    case SDL_SCANCODE_E: return 0x3;
    case SDL_SCANCODE_A: return 0x4;
    case SDL_SCANCODE_S: return 0x5;
    case SDL_SCANCODE_D: return 0x6;
    case SDL_SCANCODE_Z: return 0x7;
    case SDL_SCANCODE_X: return 0x8;
    case SDL_SCANCODE_C: return 0x9;
    case SDL_SCANCODE_R: return 0xA;
    case SDL_SCANCODE_F: return 0xB;
    case SDL_SCANCODE_V: return 0xC;
    case SDL_SCANCODE_T: return 0xD;
    case SDL_SCANCODE_G: return 0xE;
    case SDL_SCANCODE_B: return 0xF;
    default: return -1;
  }
}
