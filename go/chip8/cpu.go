// Package chip8 is the core CHIP-8 machine, independent of any windowing or audio backend.
package chip8

import (
	"fmt"
	"math/rand/v2"
)

const (
	MemorySize     = 0x1000
	ProgramStart   = 0x200
	StackDepth     = 16
	fontByteLength = 5
	addrMask       = MemorySize - 1
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

// RomTooLargeError is returned by Load when a ROM doesn't fit in program memory.
type RomTooLargeError struct {
	Size int
}

func (e *RomTooLargeError) Error() string {
	return fmt.Sprintf("ROM is %d bytes, but at most %d fit in memory", e.Size, MemorySize-ProgramStart)
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
	random     func() byte
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

func (c *CPU) Display() *Display {
	return c.display
}

// TickTimers decrements the delay and sound timers; call at 60Hz. It returns
// true while the sound timer is active and a beep should be played.
func (c *CPU) TickTimers() bool {
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
	// we're waiting for a keypress, don't advance..
	if c.waitForKey >= 0 {
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
		switch opcode {
		case 0x00E0: // clear screen
			c.display.Clear()
		case 0x00EE: // return from a subroutine
			if len(c.stack) == 0 {
				c.pc = ProgramStart
			} else {
				c.pc = c.stack[len(c.stack)-1]
				c.stack = c.stack[:len(c.stack)-1]
			}
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
	case 0xB000: // BNNN (PC = NNN + V0)
		c.pc = (nnn + uint16(c.v[0])) & addrMask
	case 0xC000: // CXNN (VX = random() & NN)
		c.v[x] = c.random() & nn
	case 0xD000:
		c.draw(x, y, n)
	case 0xE000:
		switch nn {
		case 0x9E: // EX9E (skip if key VX is pressed)
			c.skipIf(c.keypad.Key(c.v[x]))
		case 0xA1: // EXA1 (skip if key VX isn't pressed)
			c.skipIf(!c.keypad.Key(c.v[x]))
		}
	case 0xF000:
		c.xF000(x, nn)
	}
}

func (c *CPU) x8000(x, y int, n byte) {
	vx, vy := c.v[x], c.v[y]
	// VF is written after the result, so the flag wins when X is F
	switch n {
	case 0x0: // 8XY0 (VX = VY)
		c.v[x] = vy
	case 0x1: // 8XY1 (VX = VX | VY)
		c.v[x] |= vy
	case 0x2: // 8XY2 (VX = VX & VY)
		c.v[x] &= vy
	case 0x3: // 8XY3 (VX = VX ^ VY)
		c.v[x] ^= vy
	case 0x4: // 8XY4 VX += VY, with VF = carry
		c.v[x] = vx + vy
		c.v[0xF] = boolToByte(int(vx)+int(vy) > 0xFF)
	case 0x5: // 8XY5 VX -= VY, with VF = NOT borrow
		c.v[x] = vx - vy
		c.v[0xF] = boolToByte(vx >= vy)
	case 0x6: // 8XY6 VX >> 1, VF = LSB
		c.v[x] = vx >> 1
		c.v[0xF] = vx & 0x1
	case 0x7: // 8XY7 VX = VY - VX, with VF = NOT borrow
		c.v[x] = vy - vx
		c.v[0xF] = boolToByte(vy >= vx)
	case 0xE: // 8XYE VX << 1, VF = MSB
		c.v[x] = vx << 1
		c.v[0xF] = vx >> 7
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
	case 0x33: // FX33 (I[0..2] = BCD(VX))
		vx := c.v[x]
		c.memory[i&addrMask] = vx / 100
		c.memory[(i+1)&addrMask] = (vx / 10) % 10
		c.memory[(i+2)&addrMask] = vx % 10
	case 0x55: // FX55 (I[0..X] = V0..VX)
		for r := 0; r <= x; r++ {
			c.memory[(i+uint16(r))&addrMask] = c.v[r]
		}
	case 0x65: // FX65 (V0..VX = I[0..X])
		for r := 0; r <= x; r++ {
			c.v[r] = c.memory[(i+uint16(r))&addrMask]
		}
	}
}

// draw implements DXYN: draw an 8xN sprite from memory at I to (VX, VY), VF = collision.
func (c *CPU) draw(x, y int, n byte) {
	vx, vy := int(c.v[x]), int(c.v[y])
	c.v[0xF] = 0
	for yline := 0; yline < int(n); yline++ {
		data := c.memory[(c.i+uint16(yline))&addrMask]
		for xpix := 0; xpix < 8; xpix++ {
			if data&(0x80>>xpix) != 0 {
				px, py := vx+xpix, vy+yline
				if c.display.Get(px, py) {
					c.v[0xF] = 1
				}
				c.display.Flip(px, py)
			}
		}
	}
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
