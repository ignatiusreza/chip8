package frontend

import (
	"flag"
	"fmt"

	"github.com/ignatiusreza/chip8/go/chip8"
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

// Args are the command line: the ROM to run and how.
type Args struct {
	RomPath string
	Mode    Mode
	// the quirks and speed to run the ROM with, if given (non-nil), instead of the ones
	// from its settings file
	Quirks *chip8.Quirks
	Speed  *int
}

// ParseArgs parses the command line, args starting with the program name. It returns nil
// if there is nothing to run, after printing the usage; like the flag package, it exits
// after printing the usage for -h or an unknown flag.
func ParseArgs(args []string) (*Args, error) {
	flags := flag.NewFlagSet(args[0], flag.ExitOnError)
	display := flags.String("display", "tui", "where to show the display: tui (this terminal) or window")
	quirkList := flags.String("quirks", "", "comma separated presets (octo, vip, schip) and quirks\n(shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on, starting from the default")
	speedValue := flags.String("speed", "", "opcodes run per frame")
	flags.Usage = func() {
		out := flags.Output()
		fmt.Fprintf(out, "Usage : %s [-display tui|window] [-quirks LIST] [-speed N] ROMNAME\n\n", args[0])
		flags.PrintDefaults()
		fmt.Fprintf(out, "\nWithout -quirks or -speed, they are read from the ROM's .cfg file if it has one,\notherwise they default to %s and %d.\n", chip8.DefaultQuirks, chip8.DefaultSpeed)
	}
	flags.Parse(args[1:])
	if flags.NArg() < 1 {
		flags.Usage()
		return nil, nil
	}
	parsed := &Args{RomPath: flags.Arg(0)}
	var err error
	if parsed.Mode, err = parseMode(*display); err != nil {
		return nil, err
	}
	if *quirkList != "" {
		quirks, err := chip8.DefaultQuirks.Apply(*quirkList)
		if err != nil {
			return nil, err
		}
		parsed.Quirks = &quirks
	}
	if *speedValue != "" {
		speed, err := chip8.ParseSpeed(*speedValue)
		if err != nil {
			return nil, fmt.Errorf("-%w", err)
		}
		parsed.Speed = &speed
	}
	return parsed, nil
}
