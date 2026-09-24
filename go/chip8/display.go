package chip8

const (
	Width      = 64
	Height     = 32
	BuffLength = Width * Height
)

// Display is a monochrome 64x32 frame buffer.
type Display struct {
	buff        [BuffLength]bool
	invalidated bool
}

func NewDisplay() *Display {
	return &Display{invalidated: true}
}

func (d *Display) Clear() {
	d.buff = [BuffLength]bool{}
	d.invalidated = true
}

// Get returns the pixel at (x, y); off-screen pixels read as unset.
func (d *Display) Get(x, y int) bool {
	if !onScreen(x, y) {
		return false
	}
	return d.buff[x+y*Width]
}

// Flip toggles the pixel at (x, y); off-screen pixels are ignored.
func (d *Display) Flip(x, y int) {
	if !onScreen(x, y) {
		return
	}
	d.buff[x+y*Width] = !d.buff[x+y*Width]
	d.invalidated = true
}

// TakeInvalidated returns true once after the buffer has changed, so the
// frontend only redraws when needed.
func (d *Display) TakeInvalidated() bool {
	invalidated := d.invalidated
	d.invalidated = false
	return invalidated
}

func onScreen(x, y int) bool {
	return x >= 0 && x < Width && y >= 0 && y < Height
}
