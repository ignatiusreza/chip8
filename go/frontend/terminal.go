package frontend

import (
	"errors"
	"fmt"
	"os"
	"strings"
	"time"

	"golang.org/x/term"

	"github.com/ignatiusreza/chip8/go/chip8"
)

// Terminals only send key presses, repeated while the key is held. Without release
// events, a key counts as held for this long after its last press, which bridges the gap
// before the terminal starts repeating it.
const hold = 200 * time.Millisecond

// termKeymap is the character for each CHIP-8 keypad value, on the left side of a QWERTY
// keyboard.
const termKeymap = "1qweasdzxcrfvtgb"

// the display plus a one character border, and a title line above it
const (
	termColumns = chip8.Width + 2
	termRows    = chip8.Height/2 + 3
)

// terminal runs the emulator in the terminal it was started from, showing the 64x32
// display as 64x16 half-block characters.
type terminal struct {
	title     string
	input     chan []byte
	lastPress [16]time.Time
	quit      bool
	width     int
	height    int
}

// runTerminal switches the terminal to raw mode and an alternate screen, and runs the
// emulator at 60 ticks per second until Esc is pressed.
func runTerminal(e *Emulator, title string) error {
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

	t := &terminal{title: title, input: make(chan []byte, 16)}
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

// draw redraws the display if it changed or the terminal was resized. Each frame
// overwrites the last, so the terminal only needs clearing when resized.
func (t *terminal) draw(display *chip8.Display) {
	width, height, _ := term.GetSize(int(os.Stdout.Fd()))
	resized := width != t.width || height != t.height
	t.width, t.height = width, height
	if !display.TakeInvalidated() && !resized {
		return
	}

	var b strings.Builder
	if resized {
		b.WriteString("\x1b[2J")
	}
	b.WriteString("\x1b[H")
	if width < termColumns || height < termRows {
		fmt.Fprintf(&b, "Terminal too small, needs %dx%d", termColumns, termRows)
		os.Stdout.WriteString(b.String())
		return
	}

	fmt.Fprintf(&b, "%s (Esc to quit)\r\n", t.title)
	b.WriteString("┌" + strings.Repeat("─", chip8.Width) + "┐\r\n")
	// each character cell holds two pixels, one above the other
	for y := 0; y < chip8.Height; y += 2 {
		b.WriteString("│")
		for x := range chip8.Width {
			switch top, bottom := display.Get(x, y), display.Get(x, y+1); {
			case top && bottom:
				b.WriteString("█")
			case top:
				b.WriteString("▀")
			case bottom:
				b.WriteString("▄")
			default:
				b.WriteByte(' ')
			}
		}
		b.WriteString("│\r\n")
	}
	b.WriteString("└" + strings.Repeat("─", chip8.Width) + "┘")
	os.Stdout.WriteString(b.String())
}
