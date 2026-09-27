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
CPU run(std::initializer_list<unsigned short> program, int steps = -1) {
  std::vector<unsigned char> rom = assemble(program);
  CPU cpu;
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

TEST(CPUTest, WaitForKey) {
  CPU cpu = run({0xF30A, 0x6001});
  EXPECT_EQ(cpu.pc(), 0x202); // halted on the instruction after FX0A
  EXPECT_EQ(cpu.v(0), 0);
  cpu.setKey(0xB, false); // a release doesn't resume
  EXPECT_TRUE(cpu.waitingForKey());
  cpu.setKey(0xB, true);
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

  cpu = run({0x60FF, 0xE09E}); // key above 0xF is never pressed
  EXPECT_EQ(cpu.pc(), 0x204);
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
