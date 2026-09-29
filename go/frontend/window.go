package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"
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

// window runs the emulator in a desktop window. It implements ebiten.Game, which
// drives it once per 1/60s tick.
type window struct {
	emulator *Emulator
	graphic  *Graphic
}

// runWindow opens the window and runs the emulator until it is closed or Esc is pressed.
func runWindow(e *Emulator, title string) error {
	ebiten.SetTPS(tps)
	return ebiten.RunGame(&window{emulator: e, graphic: NewGraphic(title)})
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

// Draw updates the screen if invalidated.
func (w *window) Draw(screen *ebiten.Image) {
	w.graphic.Draw(screen, w.emulator.cpu.Display())
}

func (w *window) Layout(int, int) (int, int) {
	return w.graphic.Layout()
}
