package chip8

const (
	FrameWidth  = 128
	FrameHeight = 64
	FrameLength = FrameWidth * FrameHeight
)

// Display is a monochrome frame buffer, 64x32 in low resolution or 128x64 in the
// SUPER-CHIP high resolution. The frame is always kept at 128x64, with each low
// resolution pixel covering 2x2 frame pixels, so frontends can draw it the same way in
// either mode.
type Display struct {
	frame       [FrameLength]bool
	hires       bool
	invalidated bool
}

func NewDisplay() *Display {
	return &Display{invalidated: true}
}

func (d *Display) Hires() bool {
	return d.hires
}

// SetHires switches the resolution, clearing the screen.
func (d *Display) SetHires(hires bool) {
	d.hires = hires
	d.Clear()
}

// Width is the width in pixels of the current resolution.
func (d *Display) Width() int {
	return FrameWidth / d.scale()
}

// Height is the height in pixels of the current resolution.
func (d *Display) Height() int {
	return FrameHeight / d.scale()
}

func (d *Display) Clear() {
	d.frame = [FrameLength]bool{}
	d.invalidated = true
}

// Get returns the pixel at (x, y); off-screen pixels read as unset.
func (d *Display) Get(x, y int) bool {
	if !d.onScreen(x, y) {
		return false
	}
	return d.Pixel(x*d.scale(), y*d.scale())
}

// Flip toggles the pixel at (x, y); off-screen pixels are ignored.
func (d *Display) Flip(x, y int) {
	if !d.onScreen(x, y) {
		return
	}
	scale := d.scale()
	for dy := range scale {
		for dx := range scale {
			i := x*scale + dx + (y*scale+dy)*FrameWidth
			d.frame[i] = !d.frame[i]
		}
	}
	d.invalidated = true
}

// ScrollDown scrolls the screen down by n pixels of the current resolution; the pixels
// scrolled in are unset.
func (d *Display) ScrollDown(n int) {
	d.scroll(0, n)
}

// ScrollLeft scrolls the screen left by n pixels of the current resolution.
func (d *Display) ScrollLeft(n int) {
	d.scroll(-n, 0)
}

// ScrollRight scrolls the screen right by n pixels of the current resolution.
func (d *Display) ScrollRight(n int) {
	d.scroll(n, 0)
}

// Pixel returns a pixel of the 128x64 frame, for frontends.
func (d *Display) Pixel(x, y int) bool {
	return d.frame[x+y*FrameWidth]
}

// TakeInvalidated returns true once after the buffer has changed, so the
// frontend only redraws when needed.
func (d *Display) TakeInvalidated() bool {
	invalidated := d.invalidated
	d.invalidated = false
	return invalidated
}

func (d *Display) scale() int {
	if d.hires {
		return 1
	}
	return 2
}

func (d *Display) onScreen(x, y int) bool {
	return x >= 0 && x < d.Width() && y >= 0 && y < d.Height()
}

func (d *Display) scroll(dx, dy int) {
	dx, dy = dx*d.scale(), dy*d.scale()
	var scrolled [FrameLength]bool
	for y := range FrameHeight {
		for x := range FrameWidth {
			fromX, fromY := x-dx, y-dy
			if fromX >= 0 && fromX < FrameWidth && fromY >= 0 && fromY < FrameHeight {
				scrolled[x+y*FrameWidth] = d.Pixel(fromX, fromY)
			}
		}
	}
	d.frame = scrolled
	d.invalidated = true
}
