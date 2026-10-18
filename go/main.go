// Command chip8 runs a CHIP-8 ROM, in the terminal or a window, using Ebitengine for the
// window and oto for sound.
package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"github.com/ignatiusreza/chip8/go/chip8"
	"github.com/ignatiusreza/chip8/go/frontend"
)

func main() {
	display := flag.String("display", "tui", "where to show the display: tui (this terminal) or window")
	quirkList := flag.String("quirks", "none", "comma separated presets (none, vip, schip, octo) and quirks\n(shift, load-store, vf-reset, jump, wrap) to turn on")
	flag.Usage = func() {
		fmt.Fprintf(flag.CommandLine.Output(), "Usage : %s [-display tui|window] [-quirks LIST] ROMNAME\n\n", os.Args[0])
		flag.PrintDefaults()
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
	quirks, err := chip8.ParseQuirks(*quirkList)
	if err != nil {
		log.Fatal(err)
	}

	path := flag.Arg(0)
	rom, err := os.ReadFile(path)
	if err != nil {
		log.Fatalf("Could not read %s: %v", path, err)
	}
	emulator, err := frontend.New("Chip 8 : "+path, rom, mode, quirks)
	if err != nil {
		log.Fatal(err)
	}

	if err := emulator.Run(); err != nil {
		log.Fatal(err)
	}
}
