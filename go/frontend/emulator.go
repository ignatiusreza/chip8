// Package frontend wires the backend-free chip8 core to a screen (a terminal or a window),
// keyboard and speaker.
package frontend

import (
	"fmt"

	"github.com/ignatiusreza/chip8/go/chip8"
)

const (
	tps            = 60
	opcodesPerTick = 3
)

// Mode is where the display is shown and the keyboard read from.
type Mode int

const (
	ModeTerminal Mode = iota // the terminal the emulator was started from (default)
	ModeWindow               // a separate desktop window
)

// ParseMode parses a display mode name: "tui" (or "terminal"), or "window" (or "gui").
func ParseMode(s string) (Mode, error) {
	switch s {
	case "tui", "terminal":
		return ModeTerminal, nil
	case "window", "gui":
		return ModeWindow, nil
	}
	return 0, fmt.Errorf("unknown display %q, expected tui or window", s)
}

// Emulator is a CPU running a ROM, with its display, keypad and beeper hooked up.
type Emulator struct {
	cpu   *chip8.CPU
	input *Input
	sound *Sound
	title string
	mode  Mode
}

// New loads the ROM and sets up the audio device.
func New(title string, rom []byte, mode Mode) (*Emulator, error) {
	cpu := chip8.NewCPU()
	if err := cpu.Load(rom); err != nil {
		return nil, err
	}
	return &Emulator{
		cpu:   cpu,
		input: &Input{},
		sound: NewSound(),
		title: title,
		mode:  mode,
	}, nil
}

// Run opens the terminal or window screen and runs the emulator until Esc is pressed
// (or the window is closed).
func (e *Emulator) Run() error {
	if e.mode == ModeWindow {
		return runWindow(e, e.title)
	}
	return runTerminal(e, e.title)
}

// tick runs one 1/60s tick: timers, keyboard, then a few opcodes. It reports false once
// Esc was pressed.
func (e *Emulator) tick(keys Keys) bool {
	e.sound.SetPlaying(e.cpu.TickTimers())
	e.input.Update(keys, e.cpu)
	if e.input.QuitRequested() {
		return false
	}

	for range opcodesPerTick {
		e.cpu.Step()
	}
	return true
}
