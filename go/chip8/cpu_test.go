package chip8

import (
	"errors"
	"strings"
	"testing"
)

// run loads the program and executes one step per opcode.
func run(t *testing.T, program ...uint16) *CPU {
	t.Helper()
	return runWith(t, Quirks{}, program...)
}

// runWith is run, with the given quirks.
func runWith(t *testing.T, q Quirks, program ...uint16) *CPU {
	t.Helper()
	rom := make([]byte, 0, len(program)*2)
	for _, op := range program {
		rom = append(rom, byte(op>>8), byte(op))
	}
	c := NewCPU()
	c.SetQuirks(q)
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

func TestDrawWrapsStartPosition(t *testing.T) {
	// the top row of font "0" at (66, 33) starts at (2, 1)
	c := run(t, 0x6042, 0x6121, 0xA000, 0xD011)
	expect(t, "pixel (2, 1)", c.display.Get(2, 1), true)
	expect(t, "pixel (5, 1)", c.display.Get(5, 1), true)
	expect(t, "pixel (6, 1)", c.display.Get(6, 1), false)
}

func TestDrawWithVFAsCoordinate(t *testing.T) {
	// VF = 10 is read as the X coordinate before it becomes the collision flag
	c := run(t, 0x6F0A, 0x6100, 0xA000, 0xDF11)
	expect(t, "pixel (10, 0)", c.display.Get(10, 0), true)
	expect(t, "pixel (0, 0)", c.display.Get(0, 0), false)
	expect(t, "VF", c.v[0xF], 0)

	// with a pixel already lit at (10, 0), the collision is found and the rest of the
	// row is still drawn at x = 11..13. 0x8000 at 0x210 is the one-pixel sprite; it and
	// the 0x0000 before it also run as no-ops at the end.
	c = run(t, 0x620A, 0x6100, 0xA210, 0xD211, 0x6F0A, 0xA000, 0xDF11, 0x0000, 0x8000)
	expect(t, "pixel (10, 0)", c.display.Get(10, 0), false)
	expect(t, "pixel (11, 0)", c.display.Get(11, 0), true)
	expect(t, "pixel (13, 0)", c.display.Get(13, 0), true)
	expect(t, "pixel (0, 0)", c.display.Get(0, 0), false)
	expect(t, "VF", c.v[0xF], 1)
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

	// only the lowest nibble of VX selects the key, so 0x15 is key 5
	c = NewCPU()
	if err := c.Load([]byte{0x60, 0x15, 0xE0, 0x9E, 0x00, 0x00, 0xE0, 0xA1}); err != nil {
		t.Fatal(err)
	}
	c.SetKey(5, true)
	c.Step()
	c.Step()
	expect(t, "PC after EX9E", c.pc, 0x206)
	c.Step()
	expect(t, "PC after EXA1", c.pc, 0x208) // EXA1 doesn't skip either
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

func TestParseQuirks(t *testing.T) {
	q, err := ParseQuirks("octo")
	if err != nil {
		t.Fatal(err)
	}
	expect(t, "octo", q, Quirks{Shift: true, LoadStore: true, Wrap: true, JumpV0: true})

	// presets reset the quirks named before them, single quirks add to them
	q, err = ParseQuirks("wrap,vip,vf-reset")
	if err != nil {
		t.Fatal(err)
	}
	expect(t, "wrap,vip,vf-reset", q, Quirks{Shift: true, LoadStore: true, VFReset: true, VBlank: true, JumpV0: true})

	q, err = ParseQuirks("octo,schip,jump-v0")
	if err != nil {
		t.Fatal(err)
	}
	expect(t, "octo,schip,jump-v0", q, Quirks{JumpV0: true})

	if _, err := ParseQuirks("schip,bogus"); err == nil || !strings.Contains(err.Error(), `"bogus"`) {
		t.Errorf("ParseQuirks(schip,bogus) error = %v, want one naming \"bogus\"", err)
	}
}

func TestShiftQuirkShiftsVY(t *testing.T) {
	q := Quirks{Shift: true}
	c := runWith(t, q, 0x6081, 0x6103, 0x8016)
	expect(t, "V0", c.v[0], 0x01)
	expect(t, "VF", c.v[0xF], 1)

	c = runWith(t, q, 0x6001, 0x6181, 0x801E)
	expect(t, "V0", c.v[0], 0x02)
	expect(t, "VF", c.v[0xF], 1)
}

func TestLoadStoreQuirkAdvancesI(t *testing.T) {
	q := Quirks{LoadStore: true}
	expect(t, "I after FX55", runWith(t, q, 0xA300, 0xF255).i, 0x303)
	expect(t, "I after FX65", runWith(t, q, 0xA300, 0xF065).i, 0x301)
}

func TestVFResetQuirk(t *testing.T) {
	expect(t, "VF", run(t, 0x6F05, 0x8011).v[0xF], 5)

	for _, op := range []uint16{0x8011, 0x8012, 0x8013} {
		expect(t, "VF", runWith(t, Quirks{VFReset: true}, 0x6F05, op).v[0xF], 0)
	}
}

func TestVBlankQuirkWaitsForTheNextFrame(t *testing.T) {
	expect(t, "V0", run(t, 0xA000, 0xD001, 0x6001).v[0], 1)

	c := runWith(t, Quirks{VBlank: true}, 0xA000, 0xD001, 0x6001)
	expect(t, "V0", c.v[0], 0)
	expect(t, "PC", c.pc, 0x204)
	c.TickTimers()
	c.Step()
	expect(t, "V0", c.v[0], 1)
}

func TestJumpV0QuirkUsesV0(t *testing.T) {
	// B300 jumps to 0x300 + V3 like SUPER-CHIP, or to 0x300 + V0 with the quirk
	expect(t, "PC", run(t, 0x6004, 0x6302, 0xB300).pc, 0x302)
	expect(t, "PC", runWith(t, Quirks{JumpV0: true}, 0x6004, 0x6302, 0xB300).pc, 0x304)
}

func TestWrapQuirkWrapsSprites(t *testing.T) {
	// the top row of font "0" at (62, 31) is clipped after two pixels by default
	c := run(t, 0x603E, 0x611F, 0xA000, 0xD011)
	expect(t, "(63, 31)", c.display.Get(63, 31), true)
	expect(t, "(0, 31)", c.display.Get(0, 31), false)

	c = runWith(t, Quirks{Wrap: true}, 0x603E, 0x611F, 0xA000, 0xD012)
	expect(t, "(63, 31)", c.display.Get(63, 31), true)
	expect(t, "(0, 31)", c.display.Get(0, 31), true)
	expect(t, "(1, 31)", c.display.Get(1, 31), true)
	expect(t, "(2, 31)", c.display.Get(2, 31), false)
	// the second row wraps to the top
	expect(t, "(62, 0)", c.display.Get(62, 0), true)
	expect(t, "(1, 0)", c.display.Get(1, 0), true)
}

func TestHighResolution(t *testing.T) {
	c := run(t, 0x00FF)
	expect(t, "Hires()", c.display.Hires(), true)
	expect(t, "Width()", c.display.Width(), 128)
	expect(t, "Height()", c.display.Height(), 64)

	// font "0" at (100, 50) is only on screen in high resolution
	c = run(t, 0x00FF, 0x6064, 0x6132, 0xA000, 0xD015)
	expect(t, "Get(100, 50)", c.display.Get(100, 50), true)
	expect(t, "Pixel(100, 50)", c.display.Pixel(100, 50), true)
	expect(t, "Pixel(101, 51)", c.display.Pixel(101, 51), false)

	// switching back clears the screen, and low resolution pixels cover 2x2 frame pixels
	c = run(t, 0x00FF, 0xA000, 0xD015, 0x00FE, 0xD011)
	expect(t, "Hires()", c.display.Hires(), false)
	expect(t, "Width()", c.display.Width(), 64)
	expect(t, "Get(3, 0)", c.display.Get(3, 0), true)
	expect(t, "Pixel(7, 1)", c.display.Pixel(7, 1), true)
	expect(t, "Get(0, 1)", c.display.Get(0, 1), false)
}

func TestDrawLargeSprite(t *testing.T) {
	// a 16x16 sprite of 0xFF00 rows from 0x300, at (120, 60) in high resolution, clipped
	// at the edges, then drawn again
	var rom []byte
	for _, op := range []uint16{0x00FF, 0x6078, 0x613C, 0xA300, 0xD010, 0xD010} {
		rom = append(rom, byte(op>>8), byte(op))
	}
	rom = append(rom, make([]byte, 0x100-len(rom))...)
	for range 16 {
		rom = append(rom, 0xFF, 0x00)
	}
	c := NewCPU()
	if err := c.Load(rom); err != nil {
		t.Fatal(err)
	}
	for range 5 {
		c.Step()
	}
	expect(t, "Get(120, 60)", c.display.Get(120, 60), true)
	expect(t, "Get(127, 63)", c.display.Get(127, 63), true)
	expect(t, "Get(0, 60)", c.display.Get(0, 60), false)
	expect(t, "Get(120, 0)", c.display.Get(120, 0), false)
	expect(t, "VF", c.v[0xF], 0)

	c.Step()
	expect(t, "Get(120, 60)", c.display.Get(120, 60), false)
	expect(t, "VF", c.v[0xF], 1)
}

func TestScroll(t *testing.T) {
	// the top row of font "0" (4 pixels) at (8, 0)
	c := run(t, 0x6008, 0x6100, 0xA000, 0xD011, 0x00C3)
	expect(t, "Get(8, 0)", c.display.Get(8, 0), false)
	expect(t, "Get(8, 3)", c.display.Get(8, 3), true)

	c = run(t, 0x6008, 0x6100, 0xA000, 0xD011, 0x00FB)
	expect(t, "Get(8, 0)", c.display.Get(8, 0), false)
	expect(t, "Get(12, 0)", c.display.Get(12, 0), true)
	expect(t, "Get(15, 0)", c.display.Get(15, 0), true)

	c = run(t, 0x6008, 0x6100, 0xA000, 0xD011, 0x00FC)
	expect(t, "Get(4, 0)", c.display.Get(4, 0), true)
	expect(t, "Get(8, 0)", c.display.Get(8, 0), false)

	// pixels scrolled off the screen are gone
	c = run(t, 0x6000, 0x6100, 0xA000, 0xD011, 0x00FC, 0x00FB)
	expect(t, "Get(0, 0)", c.display.Get(0, 0), false)
	expect(t, "Get(4, 0)", c.display.Get(4, 0), false)

	// in high resolution, scrolling is by high resolution pixels
	c = run(t, 0x00FF, 0x6008, 0x6100, 0xA000, 0xD011, 0x00FB, 0x00C1)
	expect(t, "Get(12, 1)", c.display.Get(12, 1), true)
	expect(t, "Get(11, 1)", c.display.Get(11, 1), false)
}

func TestBigFont(t *testing.T) {
	c := run(t, 0x6007, 0xF030)
	expect(t, "I", c.i, BigFontStart+7*BigFontByteLength)
	expect(t, "top of the 7", c.memory[c.i], 0xFF)
}

func TestSaveAndRestoreFlags(t *testing.T) {
	c := run(t, 0x6001, 0x6102, 0x6203, 0xF275, 0x6000, 0x6100, 0x6200, 0xF185)
	expect(t, "V0", c.v[0], 1)
	expect(t, "V1", c.v[1], 2)
	expect(t, "V2", c.v[2], 0)
}

func TestExit(t *testing.T) {
	c := run(t, 0x00FD, 0x6001)
	expect(t, "Exited()", c.Exited(), true)
	expect(t, "PC", c.pc, 0x202)
	expect(t, "V0", c.v[0], 0)
}
