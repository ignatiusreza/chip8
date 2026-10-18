package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"

	"github.com/ignatiusreza/chip8/go/chip8"
)

// the window shows the 128x64 frame scaled 5x, so low resolution pixels are scaled 10x
const scale = 5

// keymap is the key for each CHIP-8 keypad value, laid out like the COSMAC
// VIP keypad on the left of a QWERTY keyboard:
//
//	1 2 3 4        1 2 3 C
//	Q W E R   ->   4 5 6 D
//	A S D F        7 8 9 E
//	Z X C V        A 0 B F
//
// so W A S D are 5 7 8 9, the up, left, down and right of most games
var keymap = [16]ebiten.Key{
	ebiten.KeyX,      // 0
	ebiten.KeyDigit1, // 1
	ebiten.KeyDigit2, // 2
	ebiten.KeyDigit3, // 3
	ebiten.KeyQ,      // 4
	ebiten.KeyW,      // 5
	ebiten.KeyE,      // 6
	ebiten.KeyA,      // 7
	ebiten.KeyS,      // 8
	ebiten.KeyD,      // 9
	ebiten.KeyZ,      // A
	ebiten.KeyC,      // B
	ebiten.KeyDigit4, // C
	ebiten.KeyR,      // D
	ebiten.KeyF,      // E
	ebiten.KeyV,      // F
}

// window runs the emulator in a desktop window. It implements ebiten.Game, which
// drives it once per 1/60s tick.
type window struct {
	emulator *Emulator
	image    *ebiten.Image
	pixels   []byte
}

// runWindow opens the window and runs the emulator until it is closed or Esc is pressed.
func runWindow(e *Emulator, title string) error {
	ebiten.SetTPS(tps)
	ebiten.SetWindowTitle(title)
	ebiten.SetWindowSize(chip8.FrameWidth*scale, chip8.FrameHeight*scale)
	return ebiten.RunGame(&window{
		emulator: e,
		image:    ebiten.NewImage(chip8.FrameWidth, chip8.FrameHeight),
		pixels:   make([]byte, chip8.FrameLength*4),
	})
}

// Update runs one tick with the keys held down in the window.
func (w *window) Update() error {
	var keys Keys
	for k, key := range keymap {
		keys.Down[k] = ebiten.IsKeyPressed(key)
	}
	keys.Quit = ebiten.IsKeyPressed(ebiten.KeyEscape)

	if !w.emulator.tick(keys) {
		return ebiten.Termination
	}
	return nil
}

// Draw redraws the display into the window, re-reading the frame buffer only if it changed.
func (w *window) Draw(screen *ebiten.Image) {
	if display := w.emulator.cpu.Display(); display.TakeInvalidated() {
		for y := range chip8.FrameHeight {
			for x := range chip8.FrameWidth {
				var c byte
				if display.Pixel(x, y) {
					c = 0xFF
				}
				i := (x + y*chip8.FrameWidth) * 4
				w.pixels[i], w.pixels[i+1], w.pixels[i+2], w.pixels[i+3] = c, c, c, 0xFF
			}
		}
		w.image.WritePixels(w.pixels)
	}

	op := &ebiten.DrawImageOptions{}
	op.GeoM.Scale(scale, scale)
	screen.DrawImage(w.image, op)
}

// Layout returns the window's logical size.
func (w *window) Layout(int, int) (int, int) {
	return chip8.FrameWidth * scale, chip8.FrameHeight * scale
}
