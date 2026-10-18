// Opcode tests, ported from the Go (go/chip8/cpu_test.go) and Rust (rust/src/cpu.rs)
// versions so all three emulators are checked against the same behaviour.

#include <initializer_list>
#include <vector>
#include <gtest/gtest.h>
#include "cpu.h"

namespace {

std::vector<unsigned char> assemble(std::initializer_list<unsigned short> program) {
  std::vector<unsigned char> rom;
  for(unsigned short op : program) {
    rom.push_back(op >> 8);
    rom.push_back(op & 0xFF);
  }
  return rom;
}

// loads the program and runs `steps` opcodes, one per opcode by default
CPU run(std::initializer_list<unsigned short> program, int steps = -1, const Quirks &quirks = Quirks()) {
  std::vector<unsigned char> rom = assemble(program);
  CPU cpu;
  cpu.setQuirks(quirks);
  EXPECT_TRUE(cpu.load(rom.data(), rom.size()));
  if(steps < 0) steps = program.size();
  for(int i = 0; i < steps; i++) cpu.step();
  return cpu;
}

}  // namespace

TEST(CPUTest, RejectsOversizedRom) {
  std::vector<unsigned char> rom(0xE01);
  CPU cpu;
  EXPECT_FALSE(cpu.load(rom.data(), rom.size()));

  rom.resize(0xE00);
  EXPECT_TRUE(cpu.load(rom.data(), rom.size()));
}

TEST(CPUTest, SetAndAdd) {
  CPU cpu = run({0x6A12, 0x7AFF});
  EXPECT_EQ(cpu.v(0xA), 0x11); // wraps
}

TEST(CPUTest, AddWithCarry) {
  CPU cpu = run({0x60F0, 0x6120, 0x8014});
  EXPECT_EQ(cpu.v(0), 0x10);
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, SubWithNotBorrow) {
  CPU cpu = run({0x6005, 0x6105, 0x8015});
  EXPECT_EQ(cpu.v(0), 0);
  EXPECT_EQ(cpu.v(0xF), 1);

  cpu = run({0x6001, 0x6102, 0x8015});
  EXPECT_EQ(cpu.v(0), 0xFF);
  EXPECT_EQ(cpu.v(0xF), 0);

  cpu = run({0x6005, 0x6105, 0x8017});
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, ShiftsSetVFToShiftedOutBit) {
  CPU cpu = run({0x6081, 0x800E});
  EXPECT_EQ(cpu.v(0), 0x02);
  EXPECT_EQ(cpu.v(0xF), 1);

  cpu = run({0x6003, 0x8006});
  EXPECT_EQ(cpu.v(0), 0x01);
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, FlagWinsWhenXIsF) {
  CPU cpu = run({0x6FF0, 0x6120, 0x8F14});
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, SkipNextInstruction) {
  // V1 = 1 is skipped, so the last step runs past the program
  CPU cpu = run({0x6007, 0x3007, 0x6101, 0x6201});
  EXPECT_EQ(cpu.pc(), 0x20A);
  EXPECT_EQ(cpu.v(1), 0);
  EXPECT_EQ(cpu.v(2), 1);
}

TEST(CPUTest, MachineCodeRoutineIsSkipped) {
  CPU cpu = run({0x0123, 0x6001});
  EXPECT_EQ(cpu.pc(), 0x204);
  EXPECT_EQ(cpu.v(0), 1);
}

TEST(CPUTest, CallAndReturn) {
  // 0x200: call 0x206; 0x202: V0 = 1; 0x206: return
  CPU cpu = run({0x2206, 0x6001, 0x0000, 0x00EE}, 0);
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x206);
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x202);
  cpu.step();
  EXPECT_EQ(cpu.v(0), 1);
}

TEST(CPUTest, ReturnWithEmptyStackRestartsProgram) {
  CPU cpu = run({0x6001, 0x00EE});
  EXPECT_EQ(cpu.pc(), PROGRAM_START);
}

TEST(CPUTest, CallDepthIsCapped) {
  // 0x200: call 0x200, recursing forever
  CPU cpu = run({0x2200}, Stack::CAPACITY + 1);
  EXPECT_EQ(cpu.stackSize(), Stack::CAPACITY);
  EXPECT_EQ(cpu.pc(), 0x200);
}

TEST(CPUTest, PCWraps) {
  // jump to 0xFFE, the last opcode of a full-size ROM, which sets V0
  std::vector<unsigned char> rom(CPU::MAX_ROM_SIZE);
  rom[0] = 0x1F; rom[1] = 0xFE;
  rom[rom.size() - 2] = 0x60; rom[rom.size() - 1] = 0x01;
  CPU cpu;
  ASSERT_TRUE(cpu.load(rom.data(), rom.size()));
  cpu.step();
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x000);
  EXPECT_EQ(cpu.v(0), 1);
}

TEST(CPUTest, BCD) {
  CPU cpu = run({0x60FE, 0xA300, 0xF033});
  EXPECT_EQ(cpu.memory(0x300), 2);
  EXPECT_EQ(cpu.memory(0x301), 5);
  EXPECT_EQ(cpu.memory(0x302), 4);
}

TEST(CPUTest, StoreAndLoadRegisters) {
  // store V0..V2, clear them, then load back V0..V1 only
  CPU cpu = run({0x6001, 0x6102, 0x6203, 0xA300, 0xF255, 0x6000, 0x6100, 0x6200, 0xF165});
  EXPECT_EQ(cpu.memory(0x300), 1);
  EXPECT_EQ(cpu.memory(0x301), 2);
  EXPECT_EQ(cpu.memory(0x302), 3);
  EXPECT_EQ(cpu.v(0), 1);
  EXPECT_EQ(cpu.v(1), 2);
  EXPECT_EQ(cpu.v(2), 0);
}

TEST(CPUTest, FontIsMasked) {
  CPU cpu = run({0x601F, 0xF029});
  EXPECT_EQ(cpu.index(), 0xF * FONT_BYTE_LENGTH);
}

TEST(CPUTest, DrawDetectsCollision) {
  // draw font "0" at (0, 0) once, then twice
  CPU cpu = run({0x6000, 0xF029, 0xD005});
  EXPECT_TRUE(cpu.display().get(0, 0));
  EXPECT_EQ(cpu.v(0xF), 0);

  cpu = run({0x6000, 0xF029, 0xD005, 0xD005});
  EXPECT_FALSE(cpu.display().get(0, 0));
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, DrawClipsAtRightEdge) {
  // the top row of font "0" (0xF0, 4 pixels) at x = 62: two fit, two are off-screen
  CPU cpu = run({0x603E, 0x6100, 0xA000, 0xD011});
  EXPECT_TRUE(cpu.display().get(62, 0));
  EXPECT_FALSE(cpu.display().get(0, 1));
  EXPECT_FALSE(cpu.display().get(1, 1));
}

TEST(CPUTest, DrawWrapsStartPosition) {
  // the top row of font "0" at (66, 33) starts at (2, 1)
  CPU cpu = run({0x6042, 0x6121, 0xA000, 0xD011});
  EXPECT_TRUE(cpu.display().get(2, 1));
  EXPECT_TRUE(cpu.display().get(5, 1));
  EXPECT_FALSE(cpu.display().get(6, 1));
}

TEST(CPUTest, DrawWithVFAsCoordinate) {
  // VF = 10 is read as the X coordinate before it becomes the collision flag
  CPU cpu = run({0x6F0A, 0x6100, 0xA000, 0xDF11});
  EXPECT_TRUE(cpu.display().get(10, 0));
  EXPECT_FALSE(cpu.display().get(0, 0));
  EXPECT_EQ(cpu.v(0xF), 0);

  // with a pixel already lit at (10, 0), the collision is found and the rest of the
  // row is still drawn at x = 11..13
  cpu = run({0x620A, 0x6100, 0xA210, 0xD211, 0x6F0A, 0xA000, 0xDF11, 0x0000, 0x8000}, 7);
  EXPECT_FALSE(cpu.display().get(10, 0));
  EXPECT_TRUE(cpu.display().get(11, 0));
  EXPECT_TRUE(cpu.display().get(13, 0));
  EXPECT_FALSE(cpu.display().get(0, 0));
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, WaitForKey) {
  CPU cpu = run({0xF30A, 0x6001});
  EXPECT_EQ(cpu.pc(), 0x202); // halted on the instruction after FX0A
  EXPECT_EQ(cpu.v(0), 0);
  cpu.setKey(0xB, false); // a release without a press doesn't resume
  cpu.setKey(0xB, true);
  cpu.setKey(0xC, true); // the first key pressed is the one waited for
  cpu.setKey(0xC, false);
  cpu.step();
  EXPECT_EQ(cpu.v(0), 0); // still waiting for the key to be released
  EXPECT_TRUE(cpu.waitingForKey());
  cpu.setKey(0xB, false);
  EXPECT_EQ(cpu.v(3), 0xB);
  cpu.step();
  EXPECT_EQ(cpu.v(0), 1);
}

TEST(CPUTest, KeySkips) {
  CPU cpu = run({0x6005, 0xE09E}, 0);
  cpu.setKey(5, true);
  cpu.step();
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x206);

  // only the lowest nibble of VX selects the key, so 0x15 is key 5
  cpu = run({0x6015, 0xE09E, 0x0000, 0xE0A1}, 0);
  cpu.setKey(5, true);
  cpu.step();
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x206);
  cpu.step();
  EXPECT_EQ(cpu.pc(), 0x208); // EXA1 doesn't skip either
}

TEST(CPUTest, RandomIsMaskedByNN) {
  for(int i = 0; i < 100; i++) {
    CPU cpu = run({0xC00F});
    EXPECT_EQ(cpu.v(0) & 0xF0, 0);
  }
}

TEST(CPUTest, Timers) {
  CPU cpu = run({0x6002, 0xF015, 0xF018, 0xF107}, 3);
  EXPECT_TRUE(cpu.tickTimers());
  EXPECT_TRUE(cpu.tickTimers());
  EXPECT_FALSE(cpu.tickTimers());
  cpu.step();
  EXPECT_EQ(cpu.v(1), 0);
}

TEST(CPUTest, ParseQuirks) {
  Quirks quirks;
  std::string unknown;
  EXPECT_TRUE(quirks.parse("octo", unknown));
  EXPECT_TRUE(quirks.shift && quirks.loadStore && quirks.wrap && quirks.jumpV0);
  EXPECT_FALSE(quirks.vfReset || quirks.vblank);

  // presets reset the quirks named before them, single quirks add to them
  EXPECT_TRUE(quirks.parse("wrap,vip,vf-reset", unknown));
  EXPECT_TRUE(quirks.shift && quirks.loadStore && quirks.vfReset && quirks.vblank && quirks.jumpV0);
  EXPECT_FALSE(quirks.wrap);

  EXPECT_TRUE(quirks.parse("octo,schip,jump-v0", unknown));
  EXPECT_TRUE(quirks.jumpV0);
  EXPECT_FALSE(quirks.shift || quirks.loadStore || quirks.wrap);

  EXPECT_FALSE(quirks.parse("schip,bogus", unknown));
  EXPECT_EQ(unknown, "bogus");
}

TEST(CPUTest, DescribeQuirks) {
  std::string unknown;
  for(const char *list : {"octo", "vip", "schip", "octo,vf-reset", "octo,vblank", "vip,wrap", "schip,wrap",
                           "schip,shift,load-store,jump-v0"}) {
    Quirks quirks;
    quirks.parse(list, unknown);
    EXPECT_EQ(quirks.describe(), list);
  }
  // the same quirks as vip,wrap, which needs fewer added
  Quirks quirks;
  quirks.parse("octo,vf-reset,vblank", unknown);
  EXPECT_EQ(quirks.describe(), "vip,wrap");
}

TEST(CPUTest, ShiftQuirkShiftsVY) {
  Quirks quirks;
  quirks.shift = true;
  CPU cpu = run({0x6081, 0x6103, 0x8016}, -1, quirks);
  EXPECT_EQ(cpu.v(0), 0x01);
  EXPECT_EQ(cpu.v(0xF), 1);

  cpu = run({0x6001, 0x6181, 0x801E}, -1, quirks);
  EXPECT_EQ(cpu.v(0), 0x02);
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, LoadStoreQuirkAdvancesI) {
  Quirks quirks;
  quirks.loadStore = true;
  CPU cpu = run({0xA300, 0xF255}, -1, quirks);
  EXPECT_EQ(cpu.index(), 0x303);
  cpu = run({0xA300, 0xF065}, -1, quirks);
  EXPECT_EQ(cpu.index(), 0x301);
}

TEST(CPUTest, VFResetQuirk) {
  CPU cpu = run({0x6F05, 0x8011});
  EXPECT_EQ(cpu.v(0xF), 5);

  Quirks quirks;
  quirks.vfReset = true;
  for(unsigned short op : {0x8011, 0x8012, 0x8013}) {
    cpu = run({0x6F05, op}, -1, quirks);
    EXPECT_EQ(cpu.v(0xF), 0);
  }
}

TEST(CPUTest, JumpV0QuirkUsesV0) {
  // B300 jumps to 0x300 + V3 like SUPER-CHIP, or to 0x300 + V0 with the quirk
  CPU cpu = run({0x6004, 0x6302, 0xB300});
  EXPECT_EQ(cpu.pc(), 0x302);

  Quirks quirks;
  quirks.jumpV0 = true;
  cpu = run({0x6004, 0x6302, 0xB300}, -1, quirks);
  EXPECT_EQ(cpu.pc(), 0x304);
}

TEST(CPUTest, VBlankQuirkWaitsForTheNextFrame) {
  CPU cpu = run({0xA000, 0xD001, 0x6001});
  EXPECT_EQ(cpu.v(0), 1);

  Quirks quirks;
  quirks.vblank = true;
  cpu = run({0xA000, 0xD001, 0x6001}, -1, quirks);
  EXPECT_EQ(cpu.v(0), 0);
  EXPECT_EQ(cpu.pc(), 0x204);
  cpu.tickTimers();
  cpu.step();
  EXPECT_EQ(cpu.v(0), 1);
}

TEST(CPUTest, WrapQuirkWrapsSprites) {
  // the top row of font "0" at (62, 31) is clipped after two pixels by default
  CPU cpu = run({0x603E, 0x611F, 0xA000, 0xD011});
  EXPECT_TRUE(cpu.display().get(63, 31));
  EXPECT_FALSE(cpu.display().get(0, 31));

  Quirks quirks;
  quirks.wrap = true;
  cpu = run({0x603E, 0x611F, 0xA000, 0xD012}, -1, quirks);
  EXPECT_TRUE(cpu.display().get(63, 31));
  EXPECT_TRUE(cpu.display().get(0, 31));
  EXPECT_TRUE(cpu.display().get(1, 31));
  EXPECT_FALSE(cpu.display().get(2, 31));
  // the second row wraps to the top
  EXPECT_TRUE(cpu.display().get(62, 0));
  EXPECT_TRUE(cpu.display().get(1, 0));
}

TEST(CPUTest, HighResolution) {
  CPU cpu = run({0x00FF});
  EXPECT_TRUE(cpu.display().hires());
  EXPECT_EQ(cpu.display().width(), 128);
  EXPECT_EQ(cpu.display().height(), 64);

  // font "0" at (100, 50) is only on screen in high resolution
  cpu = run({0x00FF, 0x6064, 0x6132, 0xA000, 0xD015});
  EXPECT_TRUE(cpu.display().get(100, 50));
  EXPECT_TRUE(cpu.display().pixel(100, 50));
  EXPECT_FALSE(cpu.display().pixel(101, 51));

  // switching back clears the screen, and low resolution pixels cover 2x2 frame pixels
  cpu = run({0x00FF, 0xA000, 0xD015, 0x00FE, 0xD011});
  EXPECT_FALSE(cpu.display().hires());
  EXPECT_EQ(cpu.display().width(), 64);
  EXPECT_TRUE(cpu.display().get(3, 0));
  EXPECT_TRUE(cpu.display().pixel(7, 1));
  EXPECT_FALSE(cpu.display().get(0, 1));
}

TEST(CPUTest, DrawLargeSprite) {
  // a 16x16 sprite of 0xFF00 rows from 0x300, at (120, 60) in high resolution, clipped
  // at the edges, then drawn again
  std::vector<unsigned char> rom = assemble({0x00FF, 0x6078, 0x613C, 0xA300, 0xD010, 0xD010});
  rom.resize(0x100);
  for(int row = 0; row < 16; row++) rom.insert(rom.end(), {0xFF, 0x00});
  CPU cpu;
  ASSERT_TRUE(cpu.load(rom.data(), rom.size()));
  for(int i = 0; i < 5; i++) cpu.step();
  EXPECT_TRUE(cpu.display().get(120, 60));
  EXPECT_TRUE(cpu.display().get(127, 63));
  EXPECT_FALSE(cpu.display().get(0, 60));
  EXPECT_FALSE(cpu.display().get(120, 0));
  EXPECT_EQ(cpu.v(0xF), 0);

  cpu.step();
  EXPECT_FALSE(cpu.display().get(120, 60));
  EXPECT_EQ(cpu.v(0xF), 1);
}

TEST(CPUTest, Scroll) {
  // the top row of font "0" (4 pixels) at (8, 0)
  CPU cpu = run({0x6008, 0x6100, 0xA000, 0xD011, 0x00C3});
  EXPECT_FALSE(cpu.display().get(8, 0));
  EXPECT_TRUE(cpu.display().get(8, 3));

  cpu = run({0x6008, 0x6100, 0xA000, 0xD011, 0x00FB});
  EXPECT_FALSE(cpu.display().get(8, 0));
  EXPECT_TRUE(cpu.display().get(12, 0));
  EXPECT_TRUE(cpu.display().get(15, 0));

  cpu = run({0x6008, 0x6100, 0xA000, 0xD011, 0x00FC});
  EXPECT_TRUE(cpu.display().get(4, 0));
  EXPECT_FALSE(cpu.display().get(8, 0));

  // pixels scrolled off the screen are gone
  cpu = run({0x6000, 0x6100, 0xA000, 0xD011, 0x00FC, 0x00FB});
  EXPECT_FALSE(cpu.display().get(0, 0));
  EXPECT_FALSE(cpu.display().get(4, 0));

  // in high resolution, scrolling is by high resolution pixels
  cpu = run({0x00FF, 0x6008, 0x6100, 0xA000, 0xD011, 0x00FB, 0x00C1});
  EXPECT_TRUE(cpu.display().get(12, 1));
  EXPECT_FALSE(cpu.display().get(11, 1));
}

TEST(CPUTest, BigFont) {
  CPU cpu = run({0x6007, 0xF030});
  EXPECT_EQ(cpu.index(), BIG_FONT_START + 7 * BIG_FONT_BYTE_LENGTH);
  EXPECT_EQ(cpu.memory(cpu.index()), 0xFF); // the top of the 7
}

TEST(CPUTest, SaveAndRestoreFlags) {
  CPU cpu = run({0x6001, 0x6102, 0x6203, 0xF275, 0x6000, 0x6100, 0x6200, 0xF185});
  EXPECT_EQ(cpu.v(0), 1);
  EXPECT_EQ(cpu.v(1), 2);
  EXPECT_EQ(cpu.v(2), 0);
}

TEST(CPUTest, Exit) {
  CPU cpu = run({0x00FD, 0x6001}, 2);
  EXPECT_TRUE(cpu.exited());
  EXPECT_EQ(cpu.pc(), 0x202);
  EXPECT_EQ(cpu.v(0), 0);
}
