// Command chip8 runs a CHIP-8 ROM, in the terminal or a window, using Ebitengine for the
// window and oto for sound.
package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"github.com/ignatiusreza/chip8/go/frontend"
)

func main() {
	display := flag.String("display", "tui", "where to show the display: tui (this terminal) or window")
	quirkList := flag.String("quirks", "", "comma separated presets (octo, vip, schip) and quirks\n(shift, load-store, vf-reset, jump-v0, wrap, vblank) to turn on, starting from the default")
	speedValue := flag.String("speed", "", "opcodes run per frame")
	flag.Usage = func() {
		out := flag.CommandLine.Output()
		fmt.Fprintf(out, "Usage : %s [-display tui|window] [-quirks LIST] [-speed N] ROMNAME\n\n", os.Args[0])
		flag.PrintDefaults()
		fmt.Fprintf(out, "\nWithout -quirks or -speed, they are read from the ROM's .cfg file if it has one,\notherwise they default to %s and %d.\n", frontend.DefaultQuirks, frontend.DefaultSpeed)
	}
	flag.Parse()
	if flag.NArg() < 1 {
		flag.Usage()
		return
	}
	mode, err := frontend.ParseMode(*display)
	if err != nil {
		log.Fatal(err)
	}

	path := flag.Arg(0)
	rom, err := os.ReadFile(path)
	if err != nil {
		log.Fatalf("Could not read %s: %v", path, err)
	}

	// the command line wins over the ROM's settings file, which wins over the defaults
	settings, err := frontend.LoadRomSettings(path)
	if err != nil {
		log.Fatal(err)
	}
	quirks, err := frontend.ParseQuirks(frontend.DefaultQuirks)
	if *quirkList != "" {
		quirks, err = frontend.ParseQuirks(*quirkList)
	} else if settings.Quirks != nil {
		quirks = *settings.Quirks
	}
	if err != nil {
		log.Fatal(err)
	}
	speed := frontend.DefaultSpeed
	if *speedValue != "" {
		if speed, err = frontend.ParseSpeed(*speedValue); err != nil {
			log.Fatal("-" + err.Error())
		}
	} else if settings.Speed != nil {
		speed = *settings.Speed
	}
	emulator, err := frontend.New("Chip 8 : "+path, rom, mode, quirks, speed)
	if err != nil {
		log.Fatal(err)
	}

	if err := emulator.Run(); err != nil {
		log.Fatal(err)
	}
}
