#include "cpu.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <utility>

#define MEM(addr) _memory[(addr) & (MEMORY_SIZE - 1)]
#define VX  V[((opcode & 0x0F00) >> 8)]
#define VY  V[((opcode & 0x00F0) >> 4)]
#define NNN (opcode & 0x0FFF)
#define NN  (opcode & 0x00FF)
#define N   (opcode & 0x000F)

CPU::CPU() {
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
  // and the SUPER-CHIP big font at 0x050 - 0x0B4
  unsigned char bigFont[] = {
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // 0
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
    0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C  // 9
  };
  memset(_memory, 0, sizeof(_memory));
  memcpy(_memory, font, sizeof(font));
  memcpy(&_memory[BIG_FONT_START], bigFont, sizeof(bigFont));
  memset(V, 0, sizeof(V));
  memset(_flags, 0, sizeof(_flags));
  _waitForKey = false;
  _keyDown = -1;
  _exited = false;
  _waitForFrame = false;

  PC = PROGRAM_START;
  I  = 0;
  DT = 0;
  ST = 0;

  srand(time(NULL));
}

bool Quirks::parse(const std::string &list, std::string &unknown) {
  std::istringstream names(list);
  std::string name;
  while(std::getline(names, name, ',')) {
    if(name == "octo") { *this = Quirks(); shift = loadStore = wrap = jumpV0 = true; }
    else if(name == "vip") { *this = Quirks(); shift = loadStore = vfReset = vblank = jumpV0 = true; }
    else if(name == "schip") *this = Quirks();
    else if(name == "shift") shift = true;
    else if(name == "load-store") loadStore = true;
    else if(name == "vf-reset") vfReset = true;
    else if(name == "jump-v0") jumpV0 = true;
    else if(name == "wrap") wrap = true;
    else if(name == "vblank") vblank = true;
    else {
      unknown = name;
      return false;
    }
  }
  return true;
}

bool Quirks::operator==(const Quirks &o) const {
  return shift == o.shift && loadStore == o.loadStore && vfReset == o.vfReset &&
         jumpV0 == o.jumpV0 && wrap == o.wrap && vblank == o.vblank;
}

std::string Quirks::describe() const {
  // each quirk with its name, in the order they are listed
  const std::pair<bool Quirks::*, const char *> QUIRKS[] = {
    {&Quirks::shift, "shift"}, {&Quirks::loadStore, "load-store"}, {&Quirks::vfReset, "vf-reset"},
    {&Quirks::jumpV0, "jump-v0"}, {&Quirks::wrap, "wrap"}, {&Quirks::vblank, "vblank"},
  };
  std::string best;
  int fewest = 7;
  for(const char *name : {"octo", "vip", "schip"}) {
    Quirks preset;
    std::string unknown;
    preset.parse(name, unknown);
    std::string list = name;
    int added = 0;
    bool subset = true;
    for(const auto &[quirk, quirkName] : QUIRKS) {
      if(preset.*quirk && !(this->*quirk)) subset = false;
      if(this->*quirk && !(preset.*quirk)) {
        list += std::string(",") + quirkName;
        added++;
      }
    }
    if(subset && added < fewest) {
      best = list;
      fewest = added;
    }
  }
  return best;
}

bool CPU::load(const unsigned char *rom, std::size_t size) {
  if(size > MAX_ROM_SIZE) return false;

  memcpy(&_memory[PROGRAM_START], rom, size);
  PC = PROGRAM_START;
  return true;
}

bool CPU::tickTimers() {
  _waitForFrame = false;
  if(DT > 0) DT--;
  // the beeper plays for as long as the sound timer runs
  bool playing = ST > 0;
  if(ST > 0) ST--;
  return playing;
}

void CPU::setKey(int key, bool pressed) {
  // only a change counts, so FX0A waits for a fresh press however the keys are reported
  if(key < 0 || key >= 0x10 || _keypad.key(key) == pressed) return;

  _keypad.set(key, pressed);
  // like the COSMAC VIP, FX0A waits for a key to be pressed and then released
  if(!_waitForKey) return;
  if(pressed) {
    if(_keyDown < 0) _keyDown = key;
  } else if(_keyDown == key) {
    V[_keyBuff] = key;
    _waitForKey = false;
    _keyDown = -1;
  }
}

void CPU::_x0000() {
  if((opcode & 0xFFF0) == 0x00C0) { // 00CN (scroll down N pixels)
    _display.scrollDown(N);
  } else switch(opcode) {
    case 0x00E0: // clear screen
      _display.clearScreen();
      break;
    case 0x00FB: // scroll right 4 pixels
      _display.scrollRight(4);
      break;
    case 0x00FC: // scroll left 4 pixels
      _display.scrollLeft(4);
      break;
    case 0x00FD: // exit the interpreter
      _exited = true;
      break;
    case 0x00FE: // low resolution
      _display.setHires(false);
      break;
    case 0x00FF: // high resolution
      _display.setHires(true);
      break;
    default: // 0NNN (machine code routine) is unsupported, skip it
      break;
  }
}

void CPU::_x8000() {
  X = (opcode & 0x0F00) >> 8;
  Y = (opcode & 0x00F0) >> 4;

  // VF is written after the result, so the flag wins when X is F
  switch(opcode & 0x000F) {
    case 0x0000: // 8XY0 (VX = VY)
      V[X] = V[Y];
      break;
    case 0x0001: // 8XY1 (VX = VX | VY)
      V[X] |= V[Y];
      if(_quirks.vfReset) V[0xF] = 0;
      break;
    case 0x0002: // 8XY2 (VX = VX & VY)
      V[X] &= V[Y];
      if(_quirks.vfReset) V[0xF] = 0;
      break;
    case 0x0003: // 8XY3 (VX = VX ^ VY)
      V[X] ^= V[Y];
      if(_quirks.vfReset) V[0xF] = 0;
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
      if(_quirks.shift) V[X] = V[Y];
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
      if(_quirks.shift) V[X] = V[Y];
      flag = V[X] >> 7;
      V[X] <<= 1;
      V[0xF] = flag;
      break;
  }
}

void CPU::_xF000() {
  X = (opcode & 0x0F00) >> 8;

  switch(opcode & 0x00FF) {
    case 0x0007: // FX07 (VX = DT)
      V[X] = DT;
      break;
    case 0x000A: // FX0A (wait for a key press and release, store it to VX)
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
    case 0x0030: // FX30 ( I = location of big font for value of V[X] )
      I = BIG_FONT_START + (V[X] & 0xF) * BIG_FONT_BYTE_LENGTH;
      break;
    case 0x0033: // FX33 ( I[0..2] = BCD(VX) )
      MEM(I) = V[X] / 100;
      MEM(I+1) = (V[X] / 10) % 10;
      MEM(I+2) = V[X] % 10;
      break;
    case 0x0055: // FX55 ( I[0..X] = V0..VX )
      for(int i = 0;i <= X;i++) MEM(I + i) = V[i];
      if(_quirks.loadStore) I += X + 1;
      break;
    case 0x0065: // FX65 ( V0..VX = I[0..X] )
      for(int i = 0;i <= X;i++) V[i] = MEM(I + i);
      if(_quirks.loadStore) I += X + 1;
      break;
    case 0x0075: // FX75 ( flags[0..X] = V0..VX )
      for(int i = 0;i <= X;i++) _flags[i] = V[i];
      break;
    case 0x0085: // FX85 ( V0..VX = flags[0..X] )
      for(int i = 0;i <= X;i++) V[i] = _flags[i];
      break;
  }
}

// DXYN (draw an 8xN sprite from memory at I to (VX, VY), VF = collision), or DXY0 (a
// 16x16 sprite, two bytes per row)
void CPU::_draw() {
  // read the coordinates before VF is reset, as VX or VY may be VF; the start
  // position wraps around the screen, and the sprite is clipped at its edges, or
  // wraps too with the wrap quirk
  int width = _display.width(), height = _display.height();
  int x = VX % width;
  int y = VY % height;
  int columns = N == 0 ? 16 : 8, rows = N == 0 ? 16 : N;

  V[0xF] = 0; // Reset collision flag
  for(int yline = 0; yline < rows; yline++) {
    // the pixels of a row, leftmost in the highest bit
    int data = columns == 16 ? (MEM(I + yline*2) << 8) | MEM(I + yline*2 + 1) : MEM(I + yline);
    for(int xpix = 0; xpix < columns; xpix++) {
      if((data & (1 << (columns - 1 - xpix))) != 0) {
        int px = x + xpix, py = y + yline;
        if(_quirks.wrap) { px %= width; py %= height; }
        if(_display.get(px, py)) V[0xF] = 1; //there has been a collision
        _display.flip(px, py);
      }
    }
  }
  if(_quirks.vblank) _waitForFrame = true;
}

void CPU::step() {
  // we're waiting for a key or the next frame, or the program is done, don't advance..
  if(_waitForKey || _waitForFrame || _exited) return;

  opcode = (MEM(PC) << 8) | MEM(PC+1);
  switch(opcode & 0xF000) {
    case 0x0000:
      if(opcode == 0x00EE) { // return from a subroutine
        // with nothing to return to, restart the program rather than run the font data
        if(_stack.empty()) PC = PROGRAM_START;
        else {
          PC = _stack.back();
          _stack.pop_back();
        }
      }else {
        _x0000();
        PC += 2;
      }
      break;
    case 0x1000: // 1NNN (jump to NNN)
      PC = NNN;
      break;
    case 0x2000: // 2NNN (call subroutine at NNN), the return address is dropped once the stack is full
      PC += 2;
      if(_stack.size() < STACK_DEPTH) _stack.push_back(PC);
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
    case 0xB000: // BXNN (PC = XNN + VX), or BNNN (PC = NNN + V0) with the jump-v0 quirk
      PC = (NNN + (_quirks.jumpV0 ? V[0] : VX)) & (MEMORY_SIZE - 1);
      break;
    case 0xC000: // CXNN	(VX = random() & NN)
      VX = rand() & NN;
      PC += 2;
      break;
    case 0xD000:
      _draw();
      PC += 2;
      break;
    case 0xE000:
      switch(opcode & 0xFF) {
        // only the lowest nibble of VX selects the key
        case 0x009E: // EX9E	Skips the next instruction if the key stored in VX is pressed.
          if(_keypad.key(VX & 0xF)) PC += 2;
          break;
        case 0x00A1: // EXA1	Skips the next instruction if the key stored in VX isn't pressed.
          if(!_keypad.key(VX & 0xF)) PC += 2;
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
