// Package chip8 is the core CHIP-8 machine, independent of any windowing or audio backend.
package chip8

import (
	"fmt"
	"math/rand/v2"
	"strings"
)

const (
	MemorySize     = 0x1000
	ProgramStart   = 0x200
	StackDepth     = 16
	fontByteLength = 5
	addrMask       = MemorySize - 1

	// the SUPER-CHIP 8x10 font for the digits 0-9 (FX30) follows the 4x5 font
	BigFontStart      = 0x50
	BigFontByteLength = 10
)

// store font data at 0x000 - 0x050
var font = [80]byte{
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
	0xF0, 0x80, 0xF0, 0x80, 0x80, // F
}

// and the SUPER-CHIP big font at 0x050 - 0x0B4
var bigFont = [100]byte{
	0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // 0
	0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
	0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
	0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
	0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
	0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
	0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
	0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
	0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
	0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // 9
}

// RomTooLargeError is returned by Load when a ROM doesn't fit in program memory.
type RomTooLargeError struct {
	Size int
}

func (e *RomTooLargeError) Error() string {
	return fmt.Sprintf("ROM is %d bytes, but at most %d fit in memory", e.Size, MemorySize-ProgramStart)
}

// Quirks are behaviours where CHIP-8 interpreters disagree. With all of them off (the
// default) the CPU behaves like SUPER-CHIP 1.1; each quirk turns on another interpreter's
// behaviour.
type Quirks struct {
	Shift     bool // 8XY6/8XYE shift VY into VX, instead of shifting VX in place
	LoadStore bool // FX55/FX65 leave I pointing past the last register
	VFReset   bool // 8XY1/8XY2/8XY3 reset VF to 0
	JumpV0    bool // BNNN jumps to NNN + V0, instead of XNN + VX (X being the top nibble)
	Wrap      bool // sprites wrap around the screen edges, instead of being clipped
	VBlank    bool // DXYN waits for the next frame, so at most one sprite is drawn per frame
}

// ParseQuirks parses a quirk list (see Apply), starting from no quirks.
func ParseQuirks(s string) (Quirks, error) {
	return Quirks{}.Apply(s)
}

// Apply applies a comma separated list of presets (octo, vip, schip), which set every quirk,
// and quirk names (shift, load-store, vf-reset, jump-v0, wrap, vblank), which turn one on,
// from left to right.
func (q Quirks) Apply(s string) (Quirks, error) {
	for _, name := range strings.Split(s, ",") {
		switch name {
		case "octo":
			q = Quirks{Shift: true, LoadStore: true, Wrap: true, JumpV0: true}
		case "vip":
			q = Quirks{Shift: true, LoadStore: true, VFReset: true, VBlank: true, JumpV0: true}
		case "schip":
			q = Quirks{}
		case "shift":
			q.Shift = true
		case "load-store":
			q.LoadStore = true
		case "vf-reset":
			q.VFReset = true
		case "jump-v0":
			q.JumpV0 = true
		case "wrap":
			q.Wrap = true
		case "vblank":
			q.VBlank = true
		default:
			return Quirks{}, fmt.Errorf("unknown quirk %q, expected octo, vip, schip, shift, load-store, vf-reset, jump-v0, wrap or vblank", name)
		}
	}
	return q, nil
}

type CPU struct {
	v      [16]byte
	i      uint16
	pc     uint16
	dt, st byte
	memory [MemorySize]byte
	// return addresses, holding at most StackDepth entries
	stack   []uint16
	display *Display
	keypad  Keypad
	// register waiting for a keypress (FX0A), or -1; execution halts until it's set
	waitForKey int
	// set by 00FD; execution halts for good
	exited bool
	// set by DXYN with the vblank quirk; execution halts until the next frame
	waitForFrame bool
	// the SUPER-CHIP RPL user flags (FX75/FX85)
	flags  [16]byte
	quirks Quirks
	random func() byte
}

func NewCPU() *CPU {
	c := &CPU{
		pc:         ProgramStart,
		stack:      make([]uint16, 0, StackDepth),
		display:    NewDisplay(),
		waitForKey: -1,
		random:     func() byte { return byte(rand.Uint32()) },
	}
	copy(c.memory[:], font[:])
	copy(c.memory[BigFontStart:], bigFont[:])
	return c
}

func (c *CPU) Load(rom []byte) error {
	if len(rom) > MemorySize-ProgramStart {
		return &RomTooLargeError{Size: len(rom)}
	}
	copy(c.memory[ProgramStart:], rom)
	c.pc = ProgramStart
	return nil
}

func (c *CPU) SetQuirks(q Quirks) {
	c.quirks = q
}

// Exited reports whether the program exited (00FD).
func (c *CPU) Exited() bool {
	return c.exited
}

func (c *CPU) Display() *Display {
	return c.display
}

// TickTimers starts a frame, decrementing the delay and sound timers; call at 60Hz. It
// returns true while the sound timer is active and a beep should be played.
func (c *CPU) TickTimers() bool {
	c.waitForFrame = false
	if c.dt > 0 {
		c.dt--
	}
	if c.st > 0 {
		c.st--
		return true
	}
	return false
}

func (c *CPU) SetKey(key byte, pressed bool) {
	c.keypad.Set(key, pressed)
	if pressed && c.waitForKey >= 0 {
		c.v[c.waitForKey] = key
		c.waitForKey = -1
	}
}

// Step fetches and executes a single opcode.
func (c *CPU) Step() {
	// we're waiting for a keypress or the next frame, or the program is done, don't advance..
	if c.waitForKey >= 0 || c.waitForFrame || c.exited {
		return
	}

	opcode := uint16(c.memory[c.pc&addrMask])<<8 | uint16(c.memory[(c.pc+1)&addrMask])
	c.pc = (c.pc + 2) & addrMask
	c.execute(opcode)
}

func (c *CPU) execute(opcode uint16) {
	x := int(opcode&0x0F00) >> 8
	y := int(opcode&0x00F0) >> 4
	nnn := opcode & 0x0FFF
	nn := byte(opcode & 0x00FF)
	n := byte(opcode & 0x000F)

	switch opcode & 0xF000 {
	case 0x0000:
		switch {
		case opcode&0xFFF0 == 0x00C0: // 00CN (scroll down N pixels)
			c.display.ScrollDown(int(n))
		case opcode == 0x00E0: // clear screen
			c.display.Clear()
		case opcode == 0x00EE: // return from a subroutine
			if len(c.stack) == 0 {
				c.pc = ProgramStart
			} else {
				c.pc = c.stack[len(c.stack)-1]
				c.stack = c.stack[:len(c.stack)-1]
			}
		case opcode == 0x00FB: // scroll right 4 pixels
			c.display.ScrollRight(4)
		case opcode == 0x00FC: // scroll left 4 pixels
			c.display.ScrollLeft(4)
		case opcode == 0x00FD: // exit the interpreter
			c.exited = true
		case opcode == 0x00FE: // low resolution
			c.display.SetHires(false)
		case opcode == 0x00FF: // high resolution
			c.display.SetHires(true)
		default: // 0NNN (machine code routine) is unsupported
		}
	case 0x1000: // 1NNN (jump to NNN)
		c.pc = nnn
	case 0x2000: // 2NNN (call subroutine at NNN), the return address is dropped once the stack is full
		if len(c.stack) < StackDepth {
			c.stack = append(c.stack, c.pc)
		}
		c.pc = nnn
	case 0x3000: // 3XNN (skip next inst if VX == NN)
		c.skipIf(c.v[x] == nn)
	case 0x4000: // 4XNN (skip next inst if VX != NN)
		c.skipIf(c.v[x] != nn)
	case 0x5000: // 5XY0 (skip next inst if VX == VY)
		c.skipIf(c.v[x] == c.v[y])
	case 0x6000: // 6XNN (VX = NN)
		c.v[x] = nn
	case 0x7000: // 7XNN (VX += NN)
		c.v[x] += nn
	case 0x8000:
		c.x8000(x, y, n)
	case 0x9000: // 9XY0 (skip next inst if VX != VY)
		c.skipIf(c.v[x] != c.v[y])
	case 0xA000: // ANNN (I = NNN)
		c.i = nnn
	case 0xB000: // BXNN (PC = XNN + VX), or BNNN (PC = NNN + V0) with the jump-v0 quirk
		offset := c.v[x]
		if c.quirks.JumpV0 {
			offset = c.v[0]
		}
		c.pc = (nnn + uint16(offset)) & addrMask
	case 0xC000: // CXNN (VX = random() & NN)
		c.v[x] = c.random() & nn
	case 0xD000:
		c.draw(x, y, n)
	case 0xE000:
		switch nn {
		// only the lowest nibble of VX selects the key
		case 0x9E: // EX9E (skip if key VX is pressed)
			c.skipIf(c.keypad.Key(c.v[x] & 0xF))
		case 0xA1: // EXA1 (skip if key VX isn't pressed)
			c.skipIf(!c.keypad.Key(c.v[x] & 0xF))
		}
	case 0xF000:
		c.xF000(x, nn)
	}
}

func (c *CPU) x8000(x, y int, n byte) {
	vx, vy := c.v[x], c.v[y]
	// the shift quirk shifts VY instead of VX
	shifted := vx
	if c.quirks.Shift {
		shifted = vy
	}
	// VF is written after the result, so the flag wins when X is F
	switch n {
	case 0x0: // 8XY0 (VX = VY)
		c.v[x] = vy
	case 0x1: // 8XY1 (VX = VX | VY)
		c.v[x] |= vy
		c.resetVF()
	case 0x2: // 8XY2 (VX = VX & VY)
		c.v[x] &= vy
		c.resetVF()
	case 0x3: // 8XY3 (VX = VX ^ VY)
		c.v[x] ^= vy
		c.resetVF()
	case 0x4: // 8XY4 VX += VY, with VF = carry
		c.v[x] = vx + vy
		c.v[0xF] = boolToByte(int(vx)+int(vy) > 0xFF)
	case 0x5: // 8XY5 VX -= VY, with VF = NOT borrow
		c.v[x] = vx - vy
		c.v[0xF] = boolToByte(vx >= vy)
	case 0x6: // 8XY6 VX >> 1, VF = LSB
		c.v[x] = shifted >> 1
		c.v[0xF] = shifted & 0x1
	case 0x7: // 8XY7 VX = VY - VX, with VF = NOT borrow
		c.v[x] = vy - vx
		c.v[0xF] = boolToByte(vy >= vx)
	case 0xE: // 8XYE VX << 1, VF = MSB
		c.v[x] = shifted << 1
		c.v[0xF] = shifted >> 7
	}
}

// resetVF clears VF after 8XY1/8XY2/8XY3 with the vf-reset quirk.
func (c *CPU) resetVF() {
	if c.quirks.VFReset {
		c.v[0xF] = 0
	}
}

func (c *CPU) xF000(x int, nn byte) {
	i := c.i
	switch nn {
	case 0x07: // FX07 (VX = DT)
		c.v[x] = c.dt
	case 0x0A: // FX0A (wait for keypress and store it to VX)
		c.waitForKey = x
	case 0x15: // FX15 (DT = VX)
		c.dt = c.v[x]
	case 0x18: // FX18 (ST = VX)
		c.st = c.v[x]
	case 0x1E: // FX1E (I += VX)
		c.i += uint16(c.v[x])
	case 0x29: // FX29 (I = location of font for value of VX)
		c.i = uint16(c.v[x]&0xF) * fontByteLength
	case 0x30: // FX30 (I = location of big font for value of VX)
		c.i = BigFontStart + uint16(c.v[x]&0xF)*BigFontByteLength
	case 0x33: // FX33 (I[0..2] = BCD(VX))
		vx := c.v[x]
		c.memory[i&addrMask] = vx / 100
		c.memory[(i+1)&addrMask] = (vx / 10) % 10
		c.memory[(i+2)&addrMask] = vx % 10
	case 0x55: // FX55 (I[0..X] = V0..VX)
		for r := 0; r <= x; r++ {
			c.memory[(i+uint16(r))&addrMask] = c.v[r]
		}
		c.advanceI(x)
	case 0x65: // FX65 (V0..VX = I[0..X])
		for r := 0; r <= x; r++ {
			c.v[r] = c.memory[(i+uint16(r))&addrMask]
		}
		c.advanceI(x)
	case 0x75: // FX75 (flags[0..X] = V0..VX)
		copy(c.flags[:x+1], c.v[:x+1])
	case 0x85: // FX85 (V0..VX = flags[0..X])
		copy(c.v[:x+1], c.flags[:x+1])
	}
}

// advanceI leaves I past the last register after FX55/FX65 with the load/store quirk.
func (c *CPU) advanceI(x int) {
	if c.quirks.LoadStore {
		c.i += uint16(x) + 1
	}
}

// draw implements DXYN: draw an 8xN sprite from memory at I to (VX, VY), VF = collision,
// or DXY0: a 16x16 sprite, two bytes per row.
// The start position wraps around the screen, and the sprite is clipped at its edges, or
// wraps too with the wrap quirk.
func (c *CPU) draw(x, y int, n byte) {
	width, height := c.display.Width(), c.display.Height()
	vx, vy := int(c.v[x])%width, int(c.v[y])%height
	columns, rows := 8, int(n)
	if n == 0 {
		columns, rows = 16, 16
	}
	c.v[0xF] = 0
	for yline := range rows {
		// the pixels of a row, leftmost in the highest bit
		var data uint16
		if columns == 16 {
			at := c.i + uint16(yline*2)
			data = uint16(c.memory[at&addrMask])<<8 | uint16(c.memory[(at+1)&addrMask])
		} else {
			data = uint16(c.memory[(c.i+uint16(yline))&addrMask])
		}
		for xpix := range columns {
			if data&(1<<(columns-1-xpix)) != 0 {
				px, py := vx+xpix, vy+yline
				if c.quirks.Wrap {
					px, py = px%width, py%height
				}
				if c.display.Get(px, py) {
					c.v[0xF] = 1
				}
				c.display.Flip(px, py)
			}
		}
	}
	c.waitForFrame = c.quirks.VBlank
}

func (c *CPU) skipIf(cond bool) {
	if cond {
		c.pc = (c.pc + 2) & addrMask
	}
}

func boolToByte(b bool) byte {
	if b {
		return 1
	}
	return 0
}
