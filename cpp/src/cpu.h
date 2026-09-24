#ifndef _CHIP8_CPU_H
#define	_CHIP8_CPU_H

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include "stack.h"
#include "graphic.h"
#include "sound.h"
#include "input.h"
#include "SDL.h"

#define MEMORY_SIZE 0x1000
#define PROGRAM_START 0x200
#define FONT_BYTE_LENGTH 5
#define MEM(addr) _memory[(addr) & (MEMORY_SIZE - 1)]
#define VX  V[((opcode & 0x0F00) >> 8)]
#define VY  V[((opcode & 0x00F0) >> 4)]
#define NNN (opcode & 0x0FFF)
#define NN  (opcode & 0x00FF)
#define N   (opcode & 0x000F)

class CPU {
  unsigned char V[16], data, DT, ST, flag;
  unsigned char _memory[MEMORY_SIZE], X, Y, _keyBuff;
  unsigned short PC;
  unsigned short opcode, I;
  int temp;
  bool _waitForKey, _continue;

  Stack   _stack;
  Graphic _graphic;
  Sound   _sound;
  Input   _input;

  void _x8000() {
    X = (opcode & 0x0F00) >> 8;
    Y = (opcode & 0x00F0) >> 4;

    // VF is written after the result, so the flag wins when X is F
    switch(opcode & 0x000F) {
      case 0x0000: // 8XY0 (VX = VY)
        V[X] = V[Y];
        break;
      case 0x0001: // 8XY1 (VX = VX | VY)
        V[X] |= V[Y];
        break;
      case 0x0002: // 8XY2 (VX = VX & VY)
        V[X] &= V[Y];
        break;
      case 0x0003: // 8XY3 (VX = VX ^ VY)
        V[X] ^= V[Y];
        break;
      case 0x0004: // 8XY4 VX += VY, with VF = carry
        temp = V[X] + V[Y];
        V[X] = (unsigned char)temp;
        V[0xF] = (temp > 0xFF) ? 1 : 0;
        break;
      case 0x0005: // 8XY5 VX -= VY, with VF = NOT borrow
        flag = (V[X] >= V[Y]) ? 1 : 0;
        V[X] = V[X] - V[Y];
        V[0xF] = flag;
        break;
      case 0x0006: // 8XY6 VX >> 1, VF = LSB
        flag = V[X] & 0x1;
        V[X] >>= 1;
        V[0xF] = flag;
        break;
      case 0x0007: // 8XY7 VX = VY - VX, with VF = NOT borrow
        flag = (V[Y] >= V[X]) ? 1 : 0;
        V[X] = V[Y] - V[X];
        V[0xF] = flag;
        break;
      case 0x000E: // 8XYE VX << 1, VF = MSB
        flag = V[X] >> 7;
        V[X] <<= 1;
        V[0xF] = flag;
        break;
    }
  }

  void _xF000() {
    X = (opcode & 0x0F00) >> 8;

    switch(opcode & 0x00FF) {
      case 0x0007: // FX07 (VX = DT)
        V[X] = DT;
        break;
      case 0x000A: // FX0A (Wait for keypress and store it to FX)
        _waitForKey = true;
        _keyBuff = X;
        break;
      case 0x0015: // FX15 (DT = VX)
        DT = V[X];
        break;
      case 0x0018: // FX18 (ST = VX)
        ST = V[X];
        break;
      case 0x001E: // FX1E	(I += VX)
        I += V[X];
        break;
      case 0x0029: // FX29 ( I = location of font for value of V[X] )
        I = (V[X] & 0xF) * FONT_BYTE_LENGTH;
        break;
      case 0x0033: // FX33 ( I[0..2] = BCD(VX) )
        MEM(I) = V[X] / 100;
        MEM(I+1) = (V[X] / 10) % 10;
        MEM(I+2) = V[X] % 10;
        break;
      case 0x0055: // FX55 ( I[0..X] = V0..VX )
        for(int i = 0;i <= X;i++) MEM(I + i) = V[i];
        break;
      case 0x0065: // FX65 ( V0..VX = I[0..X] )
        for(int i = 0;i <= X;i++) V[i] = MEM(I + i);
        break;
    }
  }

  void parseOpcode() {
    // we're waiting for a keypress, don't advance..
    if(_waitForKey) return;

    opcode = (MEM(PC) << 8) | MEM(PC+1);
    switch(opcode & 0xF000) {
      case 0x0000:
        if(opcode == 0x00EE) { // return from a subroutine
          PC = _stack.pop();
        }else if(opcode == 0x00E0) { // clear screen
          _graphic.clearScreen();
          PC += 2;
        }else { // 0NNN (machine code routine) is unsupported, skip it
          PC += 2;
        }
        break;
      case 0x1000: // 1NNN (jump to NNN)
        PC = NNN;
        break;
      case 0x2000: // 2NNN (call subroutine at NNN)
        // i think this mean we push current address to stack then jump to NNN
        PC += 2;
        _stack.push(PC);
        PC = NNN;
        break;
      case 0x3000: // 3XNN (skip next inst if VX == NN), which mean double tick
        if(VX == NN)
          PC += 4;
        else
          PC += 2;
        break;
      case 0x4000: // 4XNN (skip next inst if VX != NN), which mean double tick
        if(VX != NN)
          PC += 4;
        else
          PC += 2;
        break;
      case 0x5000: // 5XY0 (skip next inst if VX == VY), which mean double tick
        if(VX == VY)
          PC += 4;
        else
          PC += 2;
        break;
      case 0x6000: // 6XNN (VX = NN)
        VX = NN;
        PC += 2;
        break;
      case 0x7000: // 7XNN (VX += NN)
        VX += NN;
        PC += 2;
        break;
      case 0x8000:
        _x8000();
        PC += 2;
        break;
      case 0x9000: // 9XY0 (skip next inst if VX != VY), which mean double tick
        if(VX != VY)
          PC += 4;
        else
          PC += 2;
        break;
      case 0xA000: // ANNN (I = NNN)
        I = NNN;
        PC += 2;
        break;
      case 0xB000: // BNNN (memory PC = NNN + V0)
        PC = NNN + V[0];
        break;
      case 0xC000: // CXNN	(VX = random() & NN)
        VX = rand() & NN;
        PC += 2;
        break;
      case 0xD000: // DXYN	(Draw Sprite @(VX, VY) with dimension Nx8, with data from memory at location I )
        X = (opcode & 0x0F00) >> 8;
        Y = (opcode & 0x00F0) >> 4;

        V[0xF] = 0; // Reset collision flag
        for (int yline = 0; yline < N; yline++){
          data = MEM(I + yline); //this retreives the byte for a given line of pixels
          for(int xpix = 0; xpix < 8; xpix++){
            if ((data & (0x80 >> xpix)) != 0){
              if (_graphic.get(V[X] + xpix, V[Y] + yline)) V[0xF] = 1; //there has been a collision
              _graphic.flip(V[X] + xpix, V[Y] + yline);	//note: coordinate registers from opcode
            }
          }
        }
        PC += 2;
        break;
      case 0xE000:
        switch(opcode & 0xFF) {
          case 0x009E: // EX9E	Skips the next instruction if the key stored in VX is pressed.
            if(_input.key(VX)) PC += 2;
            break;
          case 0x00A1: // EXA1	Skips the next instruction if the key stored in VX isn't pressed.
            if(!_input.key(VX)) PC += 2;
            break;
        }
        PC += 2;
        break;
      case 0xF000:
        _xF000();
        PC += 2;
        break;
    }

    // addresses wrap around the 4KB memory
    PC &= MEMORY_SIZE - 1;
  }

  void setKey(SDLKey key, bool pressed) {
    bool validKey = true;
    unsigned char keyPressed = 0;
    switch( key ){
      case SDLK_1: keyPressed = 0x0; break;
      case SDLK_q: keyPressed = 0x1; break;
      case SDLK_w: keyPressed = 0x2; break;
      case SDLK_e: keyPressed = 0x3; break;
      case SDLK_a: keyPressed = 0x4; break;
      case SDLK_s: keyPressed = 0x5; break;
      case SDLK_d: keyPressed = 0x6; break;
      case SDLK_z: keyPressed = 0x7; break;
      case SDLK_x: keyPressed = 0x8; break;
      case SDLK_c: keyPressed = 0x9; break;
      case SDLK_r: keyPressed = 0xA; break;
      case SDLK_f: keyPressed = 0xB; break;
      case SDLK_v: keyPressed = 0xC; break;
      case SDLK_t: keyPressed = 0xD; break;
      case SDLK_g: keyPressed = 0xE; break;
      case SDLK_b: keyPressed = 0xF; break;
      case SDLK_ESCAPE: _continue = false; // fall through
      default: validKey = false;
    }

    if(validKey) {
      _input.set(keyPressed, pressed);
      // only a keypress (not a release) resumes FX0A
      if(_waitForKey && pressed) {
        V[_keyBuff] = keyPressed;
        _waitForKey = false;
      }
    }
  }

  public:
    CPU() {
      // store font data at 0x000 - 0x050
      unsigned char font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
      };
      memset(_memory, 0, sizeof(_memory));
      memcpy(_memory, font, sizeof(font));
      memset(V, 0, sizeof(V));
      _continue   = true;
      _waitForKey = false;

      PC = PROGRAM_START;
      I  = 0;
      DT = 0;
      ST = 0;

      srand(time(NULL));
    }

    bool load(char *filename) {
      std::ifstream rom(filename, std::ios_base::binary | std::ios_base::in);
      if(!rom) {
        std::cout << "Could not read " << filename << std::endl;
        return false;
      }

      const int maxSize = MEMORY_SIZE - PROGRAM_START;
      rom.read((char *) &_memory[PROGRAM_START], maxSize);
      if(rom.bad()) {
        std::cout << "Error Reading ROM" << std::endl;
        return false;
      }
      if(rom.gcount() == maxSize && rom.peek() != EOF) {
        std::cout << "ROM is too large, at most " << maxSize << " bytes fit in memory" << std::endl;
        return false;
      }

      rom.close();
      PC = PROGRAM_START;
      return true;
    }

    void tick() {
      if(DT > 0) DT--;
      // play audio for as long as the sound timer runs
      _sound.setPlaying(ST > 0);
      if(ST > 0) ST--;

      // run 3 opcode per tick
      for(int i = 0;i < 3;i++) {
        // check for keyboard state
        SDL_Event event;
        while( SDL_PollEvent( &event ) ){
          /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
          switch( event.type ){
            case SDL_KEYDOWN:
              setKey(event.key.keysym.sym, true);
              break;
            case SDL_KEYUP:
              setKey(event.key.keysym.sym, false);
              break;
            case SDL_QUIT:
              _continue = false;
              break;
          }
        }

        parseOpcode();
      }
      //parseOpcode();

      // update screen if invalidated
      _graphic.updateScreen();
    }

    bool loop() { return _continue; }
};

#endif	/* _CHIP8_CPU_H */
