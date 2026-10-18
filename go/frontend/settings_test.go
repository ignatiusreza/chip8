package frontend

import (
	"slices"
	"strings"
	"testing"
)

func TestParsesQuirksAndSpeed(t *testing.T) {
	settings, err := ParseRomSettings("# Some Game\n\n  quirks = vip,wrap \r\nspeed=200\n")
	if err != nil {
		t.Fatal(err)
	}
	if settings.Quirks == nil || !settings.Quirks.VBlank || !settings.Quirks.Wrap {
		t.Errorf("Quirks = %+v, want vip and wrap", settings.Quirks)
	}
	if settings.Speed == nil || *settings.Speed != 200 {
		t.Errorf("Speed = %v, want 200", settings.Speed)
	}
}

func TestQuirksStartFromTheDefault(t *testing.T) {
	settings, err := ParseRomSettings("quirks = vf-reset\n")
	if err != nil {
		t.Fatal(err)
	}
	if q := settings.Quirks; !q.Shift || !q.LoadStore || !q.Wrap || !q.JumpV0 || !q.VFReset {
		t.Errorf("Quirks = %+v, want octo and vf-reset", q)
	}

	settings, err = ParseRomSettings("quirks = schip,shift\n")
	if err != nil {
		t.Fatal(err)
	}
	if q := settings.Quirks; q.LoadStore || q.Wrap || q.JumpV0 || !q.Shift {
		t.Errorf("Quirks = %+v, want only shift", q)
	}
}

func TestLeavesMissingSettingsUnset(t *testing.T) {
	settings, err := ParseRomSettings("speed = 7\n")
	if err != nil {
		t.Fatal(err)
	}
	if settings.Quirks != nil {
		t.Errorf("Quirks = %+v, want nil", settings.Quirks)
	}
	if settings.Speed == nil || *settings.Speed != 7 {
		t.Errorf("Speed = %v, want 7", settings.Speed)
	}
}

func TestRejectsInvalidSettings(t *testing.T) {
	for text, want := range map[string]string{
		"quirks = octo\nspeed = fast\n": "line 2: speed must be a number from 1 to 10000",
		"quirks = bogus\n":              `line 1: unknown quirk "bogus"`,
		"colour = green\n":              `line 1: unknown setting "colour", expected quirks, speed, title, author, about or controls`,
		"speed\n":                       `line 1: expected "name = value"`,
		"speed = 0\n":                   "line 1: speed must be a number from 1 to 10000",
	} {
		if _, err := ParseRomSettings(text); err == nil || !strings.HasPrefix(err.Error(), want) {
			t.Errorf("ParseRomSettings(%q) error = %v, want %q", text, err, want)
		}
	}
}

func TestParsesHowToPlay(t *testing.T) {
	settings, err := ParseRomSettings("title = Br8kout\nauthor = SharpenedSpoon\n" +
		"about = Break the bricks.\nabout = Don't miss the ball.\n" +
		"controls = A / D: move left / right\ncontrols = E: serve\n")
	if err != nil {
		t.Fatal(err)
	}
	if settings.Title != "Br8kout" || settings.Author != "SharpenedSpoon" {
		t.Errorf("Title, Author = %q, %q, want Br8kout, SharpenedSpoon", settings.Title, settings.Author)
	}
	if !slices.Equal(settings.About, []string{"Break the bricks.", "Don't miss the ball."}) {
		t.Errorf("About = %q", settings.About)
	}
	if !slices.Equal(settings.Controls, []string{"A / D: move left / right", "E: serve"}) {
		t.Errorf("Controls = %q", settings.Controls)
	}
	if settings.Quirks != nil || settings.Speed != nil {
		t.Errorf("Quirks, Speed = %v, %v, want nil", settings.Quirks, settings.Speed)
	}
}

func TestPathReplacesTheExtension(t *testing.T) {
	for rom, want := range map[string]string{
		"roms/br8kout.ch8": "roms/br8kout.cfg",
		"roms/pong":        "roms/pong.cfg",
		"my.roms/pong":     "my.roms/pong.cfg",
	} {
		if got := RomSettingsPath(rom); got != want {
			t.Errorf("RomSettingsPath(%q) = %q, want %q", rom, got, want)
		}
	}
}
