package frontend

import (
	"fmt"
	"strings"
	"unicode/utf8"

	"github.com/ignatiusreza/chip8/go/chip8"
)

// The terminal shows the game as a title line above a bordered box of character cells,
// with the ROM's how to play next to it or below it, all centred in the terminal.
const (
	// the gap between the game and how to play beside it
	panelGap = 3
	// how narrow and wide how to play beside the game can be
	panelMinWidth = 24
	panelMaxWidth = 44
)

// gameColumns and gameRows are the terminal size the game takes at a scale: 1 for 64x16
// cells of 2x4 frame pixels, or 2 for 128x32 cells of 1x2 frame pixels.
func gameColumns(scale int) int { return chip8.FrameWidth*scale/2 + 2 }
func gameRows(scale int) int    { return chip8.FrameHeight*scale/4 + 3 }

// layout is where the game and its how to play go in the terminal; positions are 0-based.
type layout struct {
	fits         bool // false if the terminal is too small for the game
	scale        int
	gameX, gameY int // the game's title line
	panelX       int
	panelY       int
	panel        []string // how to play, wrapped to fit
}

// layoutTerminal places the game, as large as possible while how to play still fits next
// to or below it, and centres them in a columns x rows terminal.
func layoutTerminal(columns, rows int, settings RomSettings) layout {
	hasPanel := len(howToPlay(settings, panelMaxWidth)) > 0
	for _, scale := range []int{2, 1} {
		if !hasPanel {
			break
		}
		gw, gh := gameColumns(scale), gameRows(scale)
		if columns >= gw+panelGap+panelMinWidth && rows >= gh {
			panel := howToPlay(settings, min(panelMaxWidth, columns-gw-panelGap))
			return besides(columns, rows, scale, truncate(panel, rows))
		}
		if panel := howToPlay(settings, gw-2); columns >= gw && rows >= gh+1+len(panel) {
			return below(columns, rows, scale, panel)
		}
	}
	// below the normal size game, as much of how to play as fits
	if gh := gameRows(1); hasPanel && columns >= gameColumns(1) && rows >= gh+2 {
		return below(columns, rows, 1, truncate(howToPlay(settings, gameColumns(1)-2), rows-gh-1))
	}
	for _, scale := range []int{2, 1} {
		if columns >= gameColumns(scale) && rows >= gameRows(scale) {
			return below(columns, rows, scale, nil)
		}
	}
	return layout{}
}

func besides(columns, rows, scale int, panel []string) layout {
	gw, gh := gameColumns(scale), gameRows(scale)
	width, height := gw+panelGap+maxWidth(panel), max(gh, len(panel))
	x, y := (columns-width)/2, (rows-height)/2
	return layout{true, scale, x, y + (height-gh)/2, x + gw + panelGap, y + (height-len(panel))/2, panel}
}

func below(columns, rows, scale int, panel []string) layout {
	gw, gh := gameColumns(scale), gameRows(scale)
	height := gh
	if len(panel) > 0 {
		height += 1 + len(panel)
	}
	x, y := (columns-gw)/2, (rows-height)/2
	// how to play lines up with the inside of the game's border
	return layout{true, scale, x, y, x + 1, y + gh + 1, panel}
}

// howToPlay lays out the ROM's title, author, about, controls, and the quirks and speed it
// runs with, as lines of at most width characters, with blank lines between them.
func howToPlay(settings RomSettings, width int) []string {
	var lines []string
	section := func(texts []string, indent int) {
		if len(texts) == 0 {
			return
		}
		if len(lines) > 0 {
			lines = append(lines, "")
		}
		for _, text := range texts {
			lines = append(lines, wrap(text, width, indent)...)
		}
	}
	var heading []string
	if settings.Title != "" {
		heading = append(heading, settings.Title)
	}
	if settings.Author != "" {
		heading = append(heading, "by "+settings.Author)
	}
	section(heading, 0)
	section(settings.About, 0)
	if len(settings.Controls) > 0 {
		section(append([]string{"Controls"}, settings.Controls...), 2)
	}
	var running []string
	if settings.Quirks != nil {
		running = append(running, "Quirks: "+settings.Quirks.String())
	}
	if settings.Speed != nil {
		running = append(running, fmt.Sprintf("Speed: %d opcodes per frame", *settings.Speed))
	}
	section(running, 2)
	return lines
}

// wrap breaks text into lines of at most width characters at spaces, indenting the lines
// after the first, and splitting words that are too long.
func wrap(text string, width, indent int) []string {
	var lines []string
	line := ""
	for _, word := range strings.Fields(text) {
		switch {
		case line == "":
			line = word
		case utf8.RuneCountInString(line)+1+utf8.RuneCountInString(word) <= width:
			line += " " + word
		default:
			lines = append(lines, line)
			line = strings.Repeat(" ", indent) + word
		}
		for utf8.RuneCountInString(line) > width {
			cut := []rune(line)
			lines = append(lines, string(cut[:width]))
			line = strings.Repeat(" ", indent) + string(cut[width:])
		}
	}
	if line != "" {
		lines = append(lines, line)
	}
	return lines
}

func truncate(lines []string, n int) []string {
	if len(lines) > n {
		return lines[:n]
	}
	return lines
}

func maxWidth(lines []string) int {
	width := 0
	for _, line := range lines {
		width = max(width, utf8.RuneCountInString(line))
	}
	return width
}
