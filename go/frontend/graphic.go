package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"

	"github.com/ignatiusreza/chip8/go/chip8"
)

const scale = 10

// Graphic is the emulator window, showing the 64x32 display scaled 10x.
type Graphic struct {
	image  *ebiten.Image
	pixels []byte
}

func NewGraphic(title string) *Graphic {
	ebiten.SetWindowTitle(title)
	ebiten.SetWindowSize(chip8.Width*scale, chip8.Height*scale)
	return &Graphic{
		image:  ebiten.NewImage(chip8.Width, chip8.Height),
		pixels: make([]byte, chip8.BuffLength*4),
	}
}

// Draw redraws the display into the window, re-reading the frame buffer only if it changed.
func (g *Graphic) Draw(screen *ebiten.Image, display *chip8.Display) {
	if display.TakeInvalidated() {
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
		g.image.WritePixels(g.pixels)
	}

	op := &ebiten.DrawImageOptions{}
	op.GeoM.Scale(scale, scale)
	screen.DrawImage(g.image, op)
}

// Layout returns the window's logical size.
func (g *Graphic) Layout() (int, int) {
	return chip8.Width * scale, chip8.Height * scale
}
