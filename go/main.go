// Command chip8 runs a CHIP-8 ROM, in the terminal or a window, using Ebitengine for the
// window and oto for sound.
package main

import (
	"log"
	"os"

	"github.com/ignatiusreza/chip8/go/frontend"
)

func main() {
	args, err := frontend.ParseArgs(os.Args)
	if err != nil {
		log.Fatal(err)
	}
	if args == nil {
		return // printed the usage
	}
	emulator, err := frontend.New(args)
	if err != nil {
		log.Fatal(err)
	}

	if err := emulator.Run(); err != nil {
		log.Fatal(err)
	}
}
