// Package frontend wires the backend-free chip8 core to a screen (a terminal or a window),
// keyboard and speaker.
package frontend

import "github.com/ignatiusreza/chip8/go/chip8"

const tps = 60

// Emulator is a CPU running a ROM, with its display, keypad and beeper hooked up.
type Emulator struct {
	cpu   *chip8.CPU
	sound *Sound
	title string
	mode  Mode
	// opcodes run per frame
	speed int
	// the ROM's settings, with the quirks and speed it runs with, which the terminal shows
	howTo chip8.RomSettings
}

// New loads the ROM and its settings file, and sets up the audio device.
func New(args *Args) (*Emulator, error) {
	rom, err := chip8.LoadRom(args.RomPath)
	if err != nil {
		return nil, err
	}
	// the command line wins over the ROM's settings file, which wins over the defaults
	howTo := rom.Settings
	quirks, speed := chip8.DefaultQuirks, chip8.DefaultSpeed
	if args.Quirks != nil {
		quirks = *args.Quirks
	} else if howTo.Quirks != nil {
		quirks = *howTo.Quirks
	}
	if args.Speed != nil {
		speed = *args.Speed
	} else if howTo.Speed != nil {
		speed = *howTo.Speed
	}
	howTo.Quirks, howTo.Speed = &quirks, &speed

	cpu := chip8.NewCPU()
	cpu.SetQuirks(quirks)
	if err := cpu.Load(rom.Data); err != nil {
		return nil, err
	}
	return &Emulator{
		cpu:   cpu,
		sound: NewSound(),
		title: "Chip 8 : " + args.RomPath,
		mode:  args.Mode,
		speed: speed,
		howTo: howTo,
	}, nil
}

// Run opens the terminal or window screen and runs the emulator until Esc is pressed
// (or the window is closed).
func (e *Emulator) Run() error {
	if e.mode == ModeWindow {
		return runWindow(e, e.title)
	}
	return runTerminal(e, e.title, e.howTo)
}

// Keys are the keys held down this tick, as read from a window or terminal.
type Keys struct {
	Down [16]bool // down state for each CHIP-8 keypad value, 0x0 - 0xF
	Quit bool     // whether Esc was pressed
}

// tick runs one 1/60s tick: timers, keyboard, then a few opcodes. It reports false once
// Esc was pressed.
func (e *Emulator) tick(keys Keys) bool {
	e.sound.SetPlaying(e.cpu.TickTimers())
	for k, down := range keys.Down {
		e.cpu.SetKey(byte(k), down)
	}
	if keys.Quit {
		return false
	}

	for range e.speed {
		e.cpu.Step()
	}
	return true
}
