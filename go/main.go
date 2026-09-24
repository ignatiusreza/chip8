// Command chip8 runs a CHIP-8 ROM in a window, using Ebitengine for graphics, input and sound.
package main

import (
	"encoding/binary"
	"fmt"
	"log"
	"math"
	"os"
	"time"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/audio"

	"github.com/ignatiusreza/chip8/go/chip8"
)

const (
	scale          = 10
	tps            = 60
	opcodesPerTick = 3
	sampleRate     = 44100
	beepHz         = 440
	beepVolume     = 0.1
)

// keymap is the key for each CHIP-8 keypad value, on the left side of a QWERTY keyboard.
var keymap = [16]ebiten.Key{
	ebiten.KeyDigit1, // 0
	ebiten.KeyQ,      // 1
	ebiten.KeyW,      // 2
	ebiten.KeyE,      // 3
	ebiten.KeyA,      // 4
	ebiten.KeyS,      // 5
	ebiten.KeyD,      // 6
	ebiten.KeyZ,      // 7
	ebiten.KeyX,      // 8
	ebiten.KeyC,      // 9
	ebiten.KeyR,      // A
	ebiten.KeyF,      // B
	ebiten.KeyV,      // C
	ebiten.KeyT,      // D
	ebiten.KeyG,      // E
	ebiten.KeyB,      // F
}

// squareWave is an endless 440Hz square wave, as 32-bit float stereo PCM.
type squareWave struct {
	pos int
}

func (s *squareWave) Read(buf []byte) (int, error) {
	const halfPeriod = sampleRate / beepHz / 2
	const frameSize = 8 // 2 channels * 4 bytes
	n := len(buf) / frameSize * frameSize
	for i := 0; i < n; i += frameSize {
		sample := float32(beepVolume)
		if (s.pos/halfPeriod)%2 == 1 {
			sample = -sample
		}
		bits := math.Float32bits(sample)
		binary.LittleEndian.PutUint32(buf[i:], bits)
		binary.LittleEndian.PutUint32(buf[i+4:], bits)
		s.pos = (s.pos + 1) % (halfPeriod * 2)
	}
	return n, nil
}

type game struct {
	cpu    *chip8.CPU
	keys   [16]bool
	screen *ebiten.Image
	pixels []byte
	beep   *audio.Player
}

func (g *game) Update() error {
	if ebiten.IsKeyPressed(ebiten.KeyEscape) {
		return ebiten.Termination
	}

	beeping := g.cpu.TickTimers()
	if g.beep != nil {
		if beeping && !g.beep.IsPlaying() {
			g.beep.Play()
		} else if !beeping && g.beep.IsPlaying() {
			g.beep.Pause()
		}
	}

	// only report changes, so FX0A waits for a fresh keypress
	for k, key := range keymap {
		if down := ebiten.IsKeyPressed(key); down != g.keys[k] {
			g.keys[k] = down
			g.cpu.SetKey(byte(k), down)
		}
	}

	for range opcodesPerTick {
		g.cpu.Step()
	}
	return nil
}

func (g *game) Draw(screen *ebiten.Image) {
	// redraw the frame buffer only if invalidated
	if display := g.cpu.Display(); display.TakeInvalidated() {
		for y := range chip8.Height {
			for x := range chip8.Width {
				var c byte
				if display.Get(x, y) {
					c = 0xFF
				}
				i := (x + y*chip8.Width) * 4
				g.pixels[i], g.pixels[i+1], g.pixels[i+2], g.pixels[i+3] = c, c, c, 0xFF
			}
		}
		g.screen.WritePixels(g.pixels)
	}

	op := &ebiten.DrawImageOptions{}
	op.GeoM.Scale(scale, scale)
	screen.DrawImage(g.screen, op)
}

func (g *game) Layout(int, int) (int, int) {
	return chip8.Width * scale, chip8.Height * scale
}

// newBeeper returns a paused player for the beep, or nil when no audio device is available.
func newBeeper() *audio.Player {
	player, err := audio.NewContext(sampleRate).NewPlayerF32(&squareWave{})
	if err != nil {
		log.Printf("Sound disabled, could not open audio device: %v", err)
		return nil
	}
	player.SetBufferSize(50 * time.Millisecond)
	return player
}

func main() {
	if len(os.Args) < 2 {
		fmt.Printf("Usage : %s ROMNAME\n", os.Args[0])
		return
	}

	rom, err := os.ReadFile(os.Args[1])
	if err != nil {
		log.Fatalf("Could not read %s: %v", os.Args[1], err)
	}
	cpu := chip8.NewCPU()
	if err := cpu.Load(rom); err != nil {
		log.Fatal(err)
	}

	g := &game{
		cpu:    cpu,
		screen: ebiten.NewImage(chip8.Width, chip8.Height),
		pixels: make([]byte, chip8.BuffLength*4),
		beep:   newBeeper(),
	}

	ebiten.SetWindowTitle("Chip 8 : " + os.Args[1])
	ebiten.SetWindowSize(chip8.Width*scale, chip8.Height*scale)
	ebiten.SetTPS(tps)
	if err := ebiten.RunGame(g); err != nil {
		log.Fatal(err)
	}
}
