// Package frontend wires the backend-free chip8 core to a window, keyboard and speaker,
// using Ebitengine.
package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"

	"github.com/ignatiusreza/chip8/go/chip8"
)

const (
	tps            = 60
	opcodesPerTick = 3
)

// Emulator is a CPU running a ROM, with its display, keypad and beeper hooked up.
// It implements ebiten.Game, which drives it once per 1/60s tick.
type Emulator struct {
	cpu     *chip8.CPU
	graphic *Graphic
	input   *Input
	sound   *Sound
}

// New loads the ROM and sets up the window and audio device.
func New(title string, rom []byte) (*Emulator, error) {
	cpu := chip8.NewCPU()
	if err := cpu.Load(rom); err != nil {
		return nil, err
	}
	return &Emulator{
		cpu:     cpu,
		graphic: NewGraphic(title),
		input:   &Input{},
		sound:   NewSound(),
	}, nil
}

// Run opens the window and runs the emulator until it is closed or Esc is pressed.
func (e *Emulator) Run() error {
	ebiten.SetTPS(tps)
	return ebiten.RunGame(e)
}

// Update runs one tick: timers, keyboard, then a few opcodes.
func (e *Emulator) Update() error {
	e.sound.SetPlaying(e.cpu.TickTimers())
	e.input.Update(e.cpu)
	if e.input.QuitRequested() {
		return ebiten.Termination
	}

	for range opcodesPerTick {
		e.cpu.Step()
	}
	return nil
}

// Draw updates the screen if invalidated.
func (e *Emulator) Draw(screen *ebiten.Image) {
	e.graphic.Draw(screen, e.cpu.Display())
}

func (e *Emulator) Layout(int, int) (int, int) {
	return e.graphic.Layout()
}
