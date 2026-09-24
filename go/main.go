// Command chip8 runs a CHIP-8 ROM in a window, using Ebitengine for graphics, input and sound.
package main

import (
	"fmt"
	"log"
	"os"

	"github.com/ignatiusreza/chip8/go/frontend"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Printf("Usage : %s ROMNAME\n", os.Args[0])
		return
	}

	rom, err := os.ReadFile(os.Args[1])
	if err != nil {
		log.Fatalf("Could not read %s: %v", os.Args[1], err)
	}
	emulator, err := frontend.New("Chip 8 : "+os.Args[1], rom)
	if err != nil {
		log.Fatal(err)
	}

	if err := emulator.Run(); err != nil {
		log.Fatal(err)
	}
}
