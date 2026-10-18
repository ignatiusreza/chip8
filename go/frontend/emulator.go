// Package frontend wires the backend-free chip8 core to a screen (a terminal or a window),
// keyboard and speaker.
package frontend

import "github.com/ignatiusreza/chip8/go/chip8"

const tps = 60

// Emulator is a CPU running a ROM, with its display, keypad and beeper hooked up.
type Emulator struct {
	cpu     *chip8.CPU
	input   *Input
	sound   *Sound
	options *Options
}

// New loads the ROM and sets up the audio device.
func New(options *Options) (*Emulator, error) {
	cpu := chip8.NewCPU()
	cpu.SetQuirks(options.Quirks)
	if err := cpu.Load(options.Rom); err != nil {
		return nil, err
	}
	return &Emulator{
		cpu:     cpu,
		input:   &Input{},
		sound:   NewSound(),
		options: options,
	}, nil
}

// Run opens the terminal or window screen and runs the emulator until Esc is pressed
// (or the window is closed).
func (e *Emulator) Run() error {
	title := "Chip 8 : " + e.options.RomPath
	if e.options.Mode == ModeWindow {
		return runWindow(e, title)
	}
	return runTerminal(e, title, e.options.Settings)
}

// tick runs one 1/60s tick: timers, keyboard, then a few opcodes. It reports false once
// Esc was pressed.
func (e *Emulator) tick(keys Keys) bool {
	e.sound.SetPlaying(e.cpu.TickTimers())
	e.input.Update(keys, e.cpu)
	if e.input.QuitRequested() {
		return false
	}

	for range e.options.Speed {
		e.cpu.Step()
	}
	return true
}
