package frontend

import (
	"flag"
	"fmt"
	"os"

	"github.com/ignatiusreza/chip8/go/chip8"
	"github.com/ignatiusreza/chip8/go/chip8/settings"
)

// Mode is where the display is shown and the keyboard read from.
type Mode int

const (
	ModeTerminal Mode = iota // the terminal the emulator was started from (default)
	ModeWindow               // a separate desktop window
)

// parseMode parses a display mode name: "tui" (or "terminal"), or "window" (or "gui").
func parseMode(s string) (Mode, error) {
	switch s {
	case "tui", "terminal":
		return ModeTerminal, nil
	case "window", "gui":
		return ModeWindow, nil
	}
	return 0, fmt.Errorf("unknown display %q, expected tui or window", s)
}

// Options are what to run and how: the command line, with the ROM and its settings file.
type Options struct {
	RomPath string
	Rom     []byte
	Mode    Mode
	// the quirks and speed the ROM runs with: from the command line, otherwise its
	// settings file, otherwise the defaults
	Quirks chip8.Quirks
	Speed  int
	// the ROM's settings file, with those quirks and speed, for the terminal to show
	Settings settings.RomSettings
}

// ParseOptions parses the command line (args starting with the program name), then reads
// the ROM and its settings file. It returns nil if there is nothing to run, after printing
// the usage; like the flag package, it exits after printing the usage for -h or an
// unknown flag.
func ParseOptions(args []string) (*Options, error) {
	flags := flag.NewFlagSet(args[0], flag.ExitOnError)
	display := flags.String("display", "tui", "where to show the display: tui (this terminal) or window")
	quirkList := flags.String("quirks", "", "comma separated presets (octo, vip, schip) and quirks\n(shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on, starting from the default")
	speedValue := flags.String("speed", "", "opcodes run per frame")
	flags.Usage = func() {
		out := flags.Output()
		fmt.Fprintf(out, "Usage : %s [-display tui|window] [-quirks LIST] [-speed N] ROMNAME\n\n", args[0])
		flags.PrintDefaults()
		fmt.Fprintf(out, "\nWithout -quirks or -speed, they are read from the ROM's .cfg file if it has one,\notherwise they default to %s and %d.\n", settings.DefaultQuirks, settings.DefaultSpeed)
	}
	flags.Parse(args[1:])
	if flags.NArg() < 1 {
		flags.Usage()
		return nil, nil
	}
	mode, err := parseMode(*display)
	if err != nil {
		return nil, err
	}

	path := flags.Arg(0)
	rom, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("Could not read %s: %v", path, err)
	}

	// the command line wins over the ROM's settings file, which wins over the defaults
	romSettings, err := settings.Load(path)
	if err != nil {
		return nil, err
	}
	quirks, err := settings.ParseQuirks(settings.DefaultQuirks)
	if *quirkList != "" {
		quirks, err = settings.ParseQuirks(*quirkList)
	} else if romSettings.Quirks != nil {
		quirks = *romSettings.Quirks
	}
	if err != nil {
		return nil, err
	}
	speed := settings.DefaultSpeed
	if *speedValue != "" {
		if speed, err = settings.ParseSpeed(*speedValue); err != nil {
			return nil, fmt.Errorf("-%w", err)
		}
	} else if romSettings.Speed != nil {
		speed = *romSettings.Speed
	}
	romSettings.Quirks, romSettings.Speed = &quirks, &speed
	return &Options{RomPath: path, Rom: rom, Mode: mode, Quirks: quirks, Speed: speed, Settings: romSettings}, nil
}
