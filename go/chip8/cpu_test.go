package chip8

import (
	"errors"
	"testing"
)

// run loads the program and executes one step per opcode.
func run(t *testing.T, program ...uint16) *CPU {
	t.Helper()
	rom := make([]byte, 0, len(program)*2)
	for _, op := range program {
		rom = append(rom, byte(op>>8), byte(op))
	}
	c := NewCPU()
	if err := c.Load(rom); err != nil {
		t.Fatal(err)
	}
	for range program {
		c.Step()
	}
	return c
}

func expect[T comparable](t *testing.T, name string, got, want T) {
	t.Helper()
	if got != want {
		t.Errorf("%s = %v, want %v", name, got, want)
	}
}

func TestRejectsOversizedRom(t *testing.T) {
	err := NewCPU().Load(make([]byte, 0xE01))
	var tooLarge *RomTooLargeError
	if !errors.As(err, &tooLarge) || tooLarge.Size != 0xE01 {
		t.Fatalf("Load() error = %v, want RomTooLargeError{0xE01}", err)
	}
	if err := NewCPU().Load(make([]byte, 0xE00)); err != nil {
		t.Fatalf("Load() of a max-size ROM: %v", err)
	}
}

func TestSetAndAdd(t *testing.T) {
	c := run(t, 0x6A12, 0x7AFF)
	expect(t, "VA", c.v[0xA], 0x11) // wraps
}

func TestAddWithCarry(t *testing.T) {
	c := run(t, 0x60F0, 0x6120, 0x8014)
	expect(t, "V0", c.v[0], 0x10)
	expect(t, "VF", c.v[0xF], 1)
}

func TestSubWithNotBorrow(t *testing.T) {
	c := run(t, 0x6005, 0x6105, 0x8015)
	expect(t, "V0", c.v[0], 0)
	expect(t, "VF", c.v[0xF], 1)

	c = run(t, 0x6001, 0x6102, 0x8015)
	expect(t, "V0", c.v[0], 0xFF)
	expect(t, "VF", c.v[0xF], 0)

	c = run(t, 0x6005, 0x6105, 0x8017)
	expect(t, "VF", c.v[0xF], 1)
}

func TestShiftsSetVFToShiftedOutBit(t *testing.T) {
	c := run(t, 0x6081, 0x800E)
	expect(t, "V0", c.v[0], 0x02)
	expect(t, "VF", c.v[0xF], 1)

	c = run(t, 0x6003, 0x8006)
	expect(t, "V0", c.v[0], 0x01)
	expect(t, "VF", c.v[0xF], 1)
}

func TestFlagWinsWhenXIsF(t *testing.T) {
	c := run(t, 0x6FF0, 0x6120, 0x8F14)
	expect(t, "VF", c.v[0xF], 1)
}

func TestSkipNextInstruction(t *testing.T) {
	// V1 = 1 is skipped, so the last step runs past the program
	c := run(t, 0x6007, 0x3007, 0x6101, 0x6201)
	expect(t, "PC", c.pc, 0x20A)
	expect(t, "V1", c.v[1], 0)
	expect(t, "V2", c.v[2], 1)
}

func TestMachineCodeRoutineIsSkipped(t *testing.T) {
	c := run(t, 0x0123, 0x6001)
	expect(t, "PC", c.pc, 0x204)
	expect(t, "V0", c.v[0], 1)
}

func TestCallAndReturn(t *testing.T) {
	c := NewCPU()
	// 0x200: call 0x206; 0x202: V0 = 1; 0x206: return
	if err := c.Load([]byte{0x22, 0x06, 0x60, 0x01, 0x00, 0x00, 0x00, 0xEE}); err != nil {
		t.Fatal(err)
	}
	c.Step()
	expect(t, "PC after call", c.pc, 0x206)
	c.Step()
	expect(t, "PC after return", c.pc, 0x202)
	c.Step()
	expect(t, "V0", c.v[0], 1)
}

func TestReturnWithEmptyStackRestartsProgram(t *testing.T) {
	c := run(t, 0x6001, 0x00EE)
	expect(t, "PC", c.pc, ProgramStart)
}

func TestCallDepthIsCapped(t *testing.T) {
	// 0x200: call 0x200, recursing forever
	c := NewCPU()
	if err := c.Load([]byte{0x22, 0x00}); err != nil {
		t.Fatal(err)
	}
	for i := 0; i <= StackDepth; i++ {
		c.Step()
	}
	expect(t, "stack depth", len(c.stack), StackDepth)
	for _, addr := range c.stack {
		expect(t, "return address", addr, 0x202)
	}
}

func TestPCWraps(t *testing.T) {
	c := run(t, 0x1FFE)
	c.memory[0xFFE], c.memory[0xFFF] = 0x60, 0x01
	c.Step()
	expect(t, "PC", c.pc, 0x000)
	expect(t, "V0", c.v[0], 1)
}

func TestBCD(t *testing.T) {
	c := run(t, 0x60FE, 0xA300, 0xF033)
	expect(t, "memory", [3]byte(c.memory[0x300:0x303]), [3]byte{2, 5, 4})
}

func TestStoreAndLoadRegisters(t *testing.T) {
	c := run(t, 0x6001, 0x6102, 0x6203, 0xA300, 0xF255)
	expect(t, "memory", [3]byte(c.memory[0x300:0x303]), [3]byte{1, 2, 3})
	c.v = [16]byte{}
	c.execute(0xF165)
	expect(t, "V0..V2", [3]byte(c.v[:3]), [3]byte{1, 2, 0})
}

func TestFontIsMasked(t *testing.T) {
	c := run(t, 0x601F, 0xF029)
	expect(t, "I", c.i, 0xF*fontByteLength)
}

func TestDrawDetectsCollision(t *testing.T) {
	// draw font "0" at (0, 0) once, then twice
	c := run(t, 0x6000, 0xF029, 0xD005)
	expect(t, "pixel", c.display.Get(0, 0), true)
	expect(t, "VF", c.v[0xF], 0)

	c = run(t, 0x6000, 0xF029, 0xD005, 0xD005)
	expect(t, "pixel", c.display.Get(0, 0), false)
	expect(t, "VF", c.v[0xF], 1)
}

func TestDrawClipsAtRightEdge(t *testing.T) {
	// the top row of font "0" (0xF0, 4 pixels) at x = 62: two fit, two are off-screen
	c := run(t, 0x603E, 0x6100, 0xA000, 0xD011)
	expect(t, "pixel (62, 0)", c.display.Get(62, 0), true)
	expect(t, "wrapped pixel (0, 1)", c.display.Get(0, 1), false)
	expect(t, "wrapped pixel (1, 1)", c.display.Get(1, 1), false)
}

func TestWaitForKey(t *testing.T) {
	c := run(t, 0xF30A, 0x6001)
	expect(t, "PC", c.pc, 0x202) // halted on the instruction after FX0A
	expect(t, "V0", c.v[0], 0)
	c.SetKey(0xB, false) // a release doesn't resume
	expect(t, "waiting", c.waitForKey, 3)
	c.SetKey(0xB, true)
	expect(t, "V3", c.v[3], 0xB)
	c.Step()
	expect(t, "V0", c.v[0], 1)
}

func TestKeySkips(t *testing.T) {
	c := NewCPU()
	if err := c.Load([]byte{0x60, 0x05, 0xE0, 0x9E}); err != nil {
		t.Fatal(err)
	}
	c.SetKey(5, true)
	c.Step()
	c.Step()
	expect(t, "PC", c.pc, 0x206)

	c = run(t, 0x60FF, 0xE09E) // key above 0xF is never pressed
	expect(t, "PC", c.pc, 0x204)
}

func TestRandomIsMaskedByNN(t *testing.T) {
	c := NewCPU()
	c.random = func() byte { return 0xAB }
	c.execute(0xC00F)
	expect(t, "V0", c.v[0], 0x0B)
}

func TestTimers(t *testing.T) {
	c := run(t, 0x6002, 0xF015, 0xF018)
	expect(t, "beep 1", c.TickTimers(), true)
	expect(t, "beep 2", c.TickTimers(), true)
	expect(t, "beep 3", c.TickTimers(), false)
	c.execute(0xF107)
	expect(t, "V1", c.v[1], 0)
}
