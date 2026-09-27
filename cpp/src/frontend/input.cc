#include "input.h"

void Input::poll(CPU &cpu) {
  SDL_Event event;
  while( SDL_PollEvent( &event ) ){
    /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
    switch( event.type ){
      case SDL_KEYDOWN:
        if(event.key.keysym.sym == SDLK_ESCAPE) _quit = true;
        cpu.setKey(keypadValue(event.key.keysym.sym), true);
        break;
      case SDL_KEYUP:
        cpu.setKey(keypadValue(event.key.keysym.sym), false);
        break;
      case SDL_QUIT:
        _quit = true;
        break;
    }
  }
}

int Input::keypadValue(SDLKey key) {
  switch( key ){
    case SDLK_1: return 0x0;
    case SDLK_q: return 0x1;
    case SDLK_w: return 0x2;
    case SDLK_e: return 0x3;
    case SDLK_a: return 0x4;
    case SDLK_s: return 0x5;
    case SDLK_d: return 0x6;
    case SDLK_z: return 0x7;
    case SDLK_x: return 0x8;
    case SDLK_c: return 0x9;
    case SDLK_r: return 0xA;
    case SDLK_f: return 0xB;
    case SDLK_v: return 0xC;
    case SDLK_t: return 0xD;
    case SDLK_g: return 0xE;
    case SDLK_b: return 0xF;
    default: return -1;
  }
}
