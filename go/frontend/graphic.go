package frontend

import (
	"github.com/hajimehoshi/ebiten/v2"

	"github.com/ignatiusreza/chip8/go/chip8"
)

const scale = 5

// Graphic is the emulator window, showing the 128x64 frame scaled 5x, so low resolution
// pixels are scaled 10x.
type Graphic struct {
	image  *ebiten.Image
	pixels []byte
}

func NewGraphic(title string) *Graphic {
	ebiten.SetWindowTitle(title)
	ebiten.SetWindowSize(chip8.FrameWidth*scale, chip8.FrameHeight*scale)
	return &Graphic{
		image:  ebiten.NewImage(chip8.FrameWidth, chip8.FrameHeight),
		pixels: make([]byte, chip8.FrameLength*4),
	}
}

// Draw redraws the display into the window, re-reading the frame buffer only if it changed.
func (g *Graphic) Draw(screen *ebiten.Image, display *chip8.Display) {
	if display.TakeInvalidated() {
		for y := range chip8.FrameHeight {
			for x := range chip8.FrameWidth {
				var c byte
				if display.Pixel(x, y) {
					c = 0xFF
				}
				i := (x + y*chip8.FrameWidth) * 4
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
	return chip8.FrameWidth * scale, chip8.FrameHeight * scale
}
