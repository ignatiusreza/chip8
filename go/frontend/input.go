package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"

	"github.com/ignatiusreza/chip8/go/chip8"
)

// keymap is the key for each CHIP-8 keypad value, on the left side of a QWERTY keyboard.
var keymap = [16]ebiten.Key{
	ebiten.KeyDigit1, // 0
	ebiten.KeyQ,      // 1
	ebiten.KeyW,      // 2
	ebiten.KeyE,      // 3
	ebiten.KeyA,      // 4
	ebiten.KeyS,      // 5
	ebiten.KeyD,      // 6
	ebiten.KeyZ,      // 7
	ebiten.KeyX,      // 8
	ebiten.KeyC,      // 9
	ebiten.KeyR,      // A
	ebiten.KeyF,      // B
	ebiten.KeyV,      // C
	ebiten.KeyT,      // D
	ebiten.KeyG,      // E
	ebiten.KeyB,      // F
}

// Input is the keyboard state, forwarded to the CPU's keypad.
type Input struct {
	keys [16]bool
	quit bool
}

// Update reads the keyboard and reports changed keys to the CPU.
func (in *Input) Update(cpu *chip8.CPU) {
	in.quit = ebiten.IsKeyPressed(ebiten.KeyEscape)

	// only report changes, so FX0A waits for a fresh keypress
	for k, key := range keymap {
		if down := ebiten.IsKeyPressed(key); down != in.keys[k] {
			in.keys[k] = down
			cpu.SetKey(byte(k), down)
		}
	}
}

// QuitRequested reports whether Esc was pressed.
func (in *Input) QuitRequested() bool {
	return in.quit
}
