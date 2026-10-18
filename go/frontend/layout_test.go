package frontend

import (
	"slices"
	"strings"
	"testing"
)

var br8kout = RomSettings{
	Title:    "Br8kout",
	Author:   "SharpenedSpoon",
	About:    []string{"Breakout: bounce the ball off your paddle to break all the bricks."},
	Controls: []string{"A / D: move the paddle left / right"},
}

func TestHowToPlay(t *testing.T) {
	want := []string{
		"Br8kout",
		"by SharpenedSpoon",
		"",
		"Breakout: bounce the ball off",
		"your paddle to break all the",
		"bricks.",
		"",
		"Controls",
		"A / D: move the paddle left /",
		"  right",
	}
	if got := howToPlay(br8kout, 30); !slices.Equal(got, want) {
		t.Errorf("howToPlay() =\n%s\nwant\n%s", strings.Join(got, "\n"), strings.Join(want, "\n"))
	}
	if got := howToPlay(RomSettings{}, 30); got != nil {
		t.Errorf("howToPlay() of no settings = %q, want none", got)
	}
}

func TestHowToPlayShowsQuirksAndSpeed(t *testing.T) {
	settings := br8kout
	quirks, _ := ParseQuirks("vf-reset")
	speed := 7
	settings.Quirks, settings.Speed = &quirks, &speed
	lines := howToPlay(settings, 40)
	want := []string{"", "Quirks: octo,vf-reset", "Speed: 7 opcodes per frame"}
	if got := lines[len(lines)-3:]; !slices.Equal(got, want) {
		t.Errorf("howToPlay() ends with %q, want %q", got, want)
	}
}

func TestWrapSplitsLongWords(t *testing.T) {
	want := []string{"abcd", "  ef", "  gh"}
	if got := wrap("abcdefgh", 4, 2); !slices.Equal(got, want) {
		t.Errorf("wrap() = %q, want %q", got, want)
	}
}

func TestLayoutPicksTheLargestGameThatFitsWithHowToPlay(t *testing.T) {
	for _, tc := range []struct {
		columns, rows int
		scale         int
		besides       bool
	}{
		{200, 50, 2, true},  // 130 + 3 + 24 columns fit beside the large game
		{140, 60, 2, false}, // the large game is too wide to fit it beside, but it fits below
		{160, 85, 2, false}, // it fits either way, and the terminal is taller than wide, so below
		{140, 40, 1, true},  // too small for the large game, so the normal one with it beside
		{120, 68, 1, false}, // it fits either way, and the terminal is taller than wide, so below
		{100, 30, 1, true},  // it fits either way, and the terminal is wider than tall, so beside
		{70, 40, 1, false},  // too narrow to put it beside, so below
		{66, 22, 1, false},  // below, cut short
	} {
		l := layoutTerminal(tc.columns, tc.rows, br8kout)
		if !l.fits || l.scale != tc.scale || (l.panelX > l.gameX+1) != tc.besides {
			t.Errorf("%dx%d: layout = %+v, want scale %d, besides %v", tc.columns, tc.rows, l, tc.scale, tc.besides)
		}
		if bottom := max(l.gameY+gameRows(l.scale), l.panelY+len(l.panel)); bottom > tc.rows {
			t.Errorf("%dx%d: layout reaches row %d", tc.columns, tc.rows, bottom)
		}
		if right := max(l.gameX+gameColumns(l.scale), l.panelX+maxWidth(l.panel)); right > tc.columns {
			t.Errorf("%dx%d: layout reaches column %d", tc.columns, tc.rows, right)
		}
	}
}

func TestLayoutCentres(t *testing.T) {
	// the normal size game alone, 66x19, in the middle of 100x29
	l := layoutTerminal(100, 29, RomSettings{})
	if l.scale != 1 || l.gameX != 17 || l.gameY != 5 || l.panel != nil {
		t.Errorf("layout = %+v, want the normal size game at (17, 5)", l)
	}

	// beside: the game and how to play, 66 + 3 + 29 columns, centred together
	l = layoutTerminal(100, 29, br8kout)
	if l.gameX != 1 || l.panelX != 70 || l.gameY != 5 || l.panelY != 9 {
		t.Errorf("layout = %+v, want the game at (1, 5) and how to play at (70, 9)", l)
	}
}

func TestLayoutTooSmall(t *testing.T) {
	if l := layoutTerminal(65, 40, br8kout); l.fits {
		t.Errorf("layout = %+v, want it not to fit", l)
	}
	if l := layoutTerminal(66, 18, RomSettings{}); l.fits {
		t.Errorf("layout = %+v, want it not to fit", l)
	}
}
