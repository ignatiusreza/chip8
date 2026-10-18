package chip8

import (
	"errors"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// DefaultQuirks are the quirks used for a ROM that doesn't say otherwise, which quirk lists
// on the command line and in settings files start from, so e.g. "vf-reset" adds that quirk
// to them.
var DefaultQuirks, _ = ParseQuirks("octo")

const (
	// DefaultSpeed is the opcodes run per frame for a ROM that doesn't say otherwise.
	DefaultSpeed = 15
	MaxSpeed     = 10000
)

// RomSettings is what a ROM needs to run well, and how to play it, read from the settings
// file next to it: the ROM's path with its extension replaced by ".cfg". Each line holds
// "name = value":
//
//	quirks = LIST     the same list as -quirks
//	speed = N         opcodes per frame
//	title = TEXT      the game's name
//	author = TEXT     who made it
//	about = TEXT      what the game is and how to win, a line of text per setting
//	controls = TEXT   a key and what it does, e.g. "A / D: move left / right", per setting
//
// Blank lines and lines starting with "#" are ignored. Quirks and speed that aren't given
// (nil) use the defaults.
type RomSettings struct {
	Quirks   *Quirks
	Speed    *int
	Title    string
	Author   string
	About    []string
	Controls []string
}

// ParseSpeed parses an opcodes per frame count, between 1 and MaxSpeed.
func ParseSpeed(value string) (int, error) {
	speed, err := strconv.Atoi(value)
	if err != nil || speed < 1 || speed > MaxSpeed {
		return 0, fmt.Errorf("speed must be a number from 1 to %d", MaxSpeed)
	}
	return speed, nil
}

// ParseRomSettings parses the text of a settings file.
func ParseRomSettings(text string) (RomSettings, error) {
	var settings RomSettings
	for i, line := range strings.Split(text, "\n") {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		name, value, ok := strings.Cut(line, "=")
		if !ok {
			return RomSettings{}, fmt.Errorf("line %d: expected \"name = value\"", i+1)
		}
		switch name, value = strings.TrimSpace(name), strings.TrimSpace(value); name {
		case "quirks":
			quirks, err := DefaultQuirks.Apply(value)
			if err != nil {
				return RomSettings{}, fmt.Errorf("line %d: %w", i+1, err)
			}
			settings.Quirks = &quirks
		case "speed":
			speed, err := ParseSpeed(value)
			if err != nil {
				return RomSettings{}, fmt.Errorf("line %d: %w", i+1, err)
			}
			settings.Speed = &speed
		case "title":
			settings.Title = value
		case "author":
			settings.Author = value
		case "about":
			settings.About = append(settings.About, value)
		case "controls":
			settings.Controls = append(settings.Controls, value)
		default:
			return RomSettings{}, fmt.Errorf("line %d: unknown setting %q, expected quirks, speed, title, author, about or controls", i+1, name)
		}
	}
	return settings, nil
}

// RomSettingsPath returns the path of the settings file for the ROM at romPath.
func RomSettingsPath(romPath string) string {
	return strings.TrimSuffix(romPath, filepath.Ext(romPath)) + ".cfg"
}

// LoadRomSettings reads the settings file for the ROM at romPath, if it has one.
func LoadRomSettings(romPath string) (RomSettings, error) {
	path := RomSettingsPath(romPath)
	text, err := os.ReadFile(path)
	if errors.Is(err, fs.ErrNotExist) {
		return RomSettings{}, nil // the ROM runs with the defaults
	}
	if err != nil {
		return RomSettings{}, fmt.Errorf("could not read %s: %w", path, err)
	}
	settings, err := ParseRomSettings(string(text))
	if err != nil {
		return RomSettings{}, fmt.Errorf("%s, %w", path, err)
	}
	return settings, nil
}
