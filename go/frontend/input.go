package frontend

import (
	"github.com/ignatiusreza/chip8/go/chip8"
)

// Keys are the keys held down this tick, as read from a window or terminal.
type Keys struct {
	Down [16]bool // down state for each CHIP-8 keypad value, 0x0 - 0xF
	Quit bool     // whether Esc was pressed
}

// Input is the keyboard state, forwarded to the CPU's keypad.
type Input struct {
	keys [16]bool
	quit bool
}

// Update reports keys that changed since the last tick to the CPU.
func (in *Input) Update(keys Keys, cpu *chip8.CPU) {
	in.quit = keys.Quit

	// only report changes, so FX0A waits for a fresh keypress
	for k, down := range keys.Down {
		if down != in.keys[k] {
			in.keys[k] = down
			cpu.SetKey(byte(k), down)
		}
	}
}

// QuitRequested reports whether Esc was pressed.
func (in *Input) QuitRequested() bool {
	return in.quit
}
