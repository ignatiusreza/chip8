package frontend

import (
	"errors"
	"fmt"
	"os"
	"strings"
	"time"

	"golang.org/x/term"

	"github.com/ignatiusreza/chip8/go/chip8"
	"github.com/ignatiusreza/chip8/go/chip8/settings"
)

// Terminals only send key presses, repeated while the key is held. Without release
// events, a key counts as held for this long after its last press, which bridges the gap
// before the terminal starts repeating it.
const hold = 200 * time.Millisecond

// termKeymap is the character for each CHIP-8 keypad value, laid out like the COSMAC
// VIP keypad on the left of a QWERTY keyboard:
//
//	1 2 3 4        1 2 3 C
//	Q W E R   ->   4 5 6 D
//	A S D F        7 8 9 E
//	Z X C V        A 0 B F
//
// so W A S D are 5 7 8 9, the up, left, down and right of most games
const termKeymap = "x123qweasdzc4rfv"

// terminal runs the emulator in the terminal it was started from, showing the display
// and the ROM's how to play, centred.
type terminal struct {
	title     string
	howTo     settings.RomSettings
	input     chan []byte
	lastPress [16]time.Time
	quit      bool
	width     int
	height    int
	layout    layout
}

// runTerminal switches the terminal to raw mode and an alternate screen, and runs the
// emulator at 60 ticks per second until Esc is pressed.
func runTerminal(e *Emulator, title string, howTo settings.RomSettings) error {
	in, out := int(os.Stdin.Fd()), int(os.Stdout.Fd())
	if !term.IsTerminal(in) || !term.IsTerminal(out) {
		return errors.New("the terminal display needs an interactive terminal, try -display window")
	}

	state, err := term.MakeRaw(in)
	if err != nil {
		return err
	}
	defer term.Restore(in, state)
	fmt.Printf("\x1b[?1049h\x1b[?25l\x1b]0;%s\a", title)
	defer fmt.Print("\x1b[?25h\x1b[?1049l")

	t := &terminal{title: title, howTo: howTo, input: make(chan []byte, 16)}
	go t.read()

	ticker := time.NewTicker(time.Second / tps)
	defer ticker.Stop()
	for range ticker.C {
		if !e.tick(t.keys()) {
			return nil
		}
		t.draw(e.cpu.Display())
	}
	return nil
}

// read forwards everything typed to the input channel.
func (t *terminal) read() {
	for {
		buf := make([]byte, 64)
		n, err := os.Stdin.Read(buf)
		if err != nil {
			return
		}
		t.input <- buf[:n]
	}
}

// keys handles everything typed since the last tick, and returns the keys held down.
func (t *terminal) keys() Keys {
	for drained := false; !drained; {
		select {
		case buf := <-t.input:
			t.handle(buf)
		default:
			drained = true
		}
	}

	now := time.Now()
	keys := Keys{Quit: t.quit}
	for k, pressed := range t.lastPress {
		keys.Down[k] = !pressed.IsZero() && now.Sub(pressed) < hold
	}
	return keys
}

func (t *terminal) handle(buf []byte) {
	for i := 0; i < len(buf); i++ {
		switch c := buf[i]; {
		case c == 0x03: // Ctrl-C
			t.quit = true
		case c == 0x1b && i == len(buf)-1: // a lone Esc
			t.quit = true
		case c == 0x1b && (buf[i+1] == '[' || buf[i+1] == 'O'):
			// skip escape sequences, such as arrow keys, up to their final byte
			for i += 2; i < len(buf) && (buf[i] < 0x40 || buf[i] > 0x7e); i++ {
			}
		default:
			if k := strings.IndexByte(termKeymap, lower(c)); k >= 0 {
				t.lastPress[k] = time.Now()
			}
		}
	}
}

func lower(c byte) byte {
	if c >= 'A' && c <= 'Z' {
		return c + 'a' - 'A'
	}
	return c
}

// draw redraws the display if it changed or the terminal was resized. The title, border
// and how to play are only drawn when the terminal is resized, which clears it.
func (t *terminal) draw(display *chip8.Display) {
	width, height, _ := term.GetSize(int(os.Stdout.Fd()))
	resized := width != t.width || height != t.height
	t.width, t.height = width, height
	if !display.TakeInvalidated() && !resized {
		return
	}

	var b strings.Builder
	l := &t.layout
	if resized {
		*l = layoutTerminal(width, height, t.howTo)
		b.WriteString("\x1b[2J")
		if !l.fits {
			fmt.Fprintf(&b, "\x1b[HTerminal too small, needs %dx%d", gameColumns(1), gameRows(1))
			os.Stdout.WriteString(b.String())
			return
		}
		cells := gameColumns(l.scale) - 2
		title := []rune(t.title + " (Esc to quit)")
		moveTo(&b, l.gameX, l.gameY)
		b.WriteString(string(title[:min(len(title), cells+2)]))
		moveTo(&b, l.gameX, l.gameY+1)
		b.WriteString("┌" + strings.Repeat("─", cells) + "┐")
		for row := range gameRows(l.scale) - 3 {
			moveTo(&b, l.gameX, l.gameY+2+row)
			b.WriteString("│")
			moveTo(&b, l.gameX+cells+1, l.gameY+2+row)
			b.WriteString("│")
		}
		moveTo(&b, l.gameX, l.gameY+gameRows(l.scale)-1)
		b.WriteString("└" + strings.Repeat("─", cells) + "┘")
		for i, line := range l.panel {
			moveTo(&b, l.panelX, l.panelY+i)
			b.WriteString(line)
		}
	} else if !l.fits {
		return
	}

	for row := range gameRows(l.scale) - 3 {
		moveTo(&b, l.gameX+1, l.gameY+2+row)
		for col := range gameColumns(l.scale) - 2 {
			switch {
			case l.scale == 2:
				// two frame pixels, one above the other
				b.WriteString(halfBlock(display.Pixel(col, row*2), display.Pixel(col, row*2+1)))
			case display.Hires():
				b.WriteRune(braille(display, col*2, row*4))
			default:
				// two low resolution pixels, one above the other
				b.WriteString(halfBlock(display.Get(col, row*2), display.Get(col, row*2+1)))
			}
		}
	}
	os.Stdout.WriteString(b.String())
}

// moveTo moves the cursor to the 0-based column x and row y.
func moveTo(b *strings.Builder, x, y int) {
	fmt.Fprintf(b, "\x1b[%d;%dH", y+1, x+1)
}

// braille is the braille character showing the 2x4 high resolution pixels at (x, y).
func braille(display *chip8.Display, x, y int) rune {
	// the bit of each dot, by column then row
	dots := [2][4]rune{{0x01, 0x02, 0x04, 0x40}, {0x08, 0x10, 0x20, 0x80}}
	bits := rune(0)
	for dx, column := range dots {
		for dy, bit := range column {
			if display.Pixel(x+dx, y+dy) {
				bits |= bit
			}
		}
	}
	return 0x2800 + bits
}

// halfBlock is the half block showing two pixels, one above the other.
func halfBlock(top, bottom bool) string {
	switch {
	case top && bottom:
		return "█"
	case top:
		return "▀"
	case bottom:
		return "▄"
	}
	return " "
}
