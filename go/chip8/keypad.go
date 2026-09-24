package chip8

// Keypad is the state of the 16-key hexadecimal keypad.
type Keypad struct {
	state uint16
}

// Key reports whether key is pressed; keys outside 0x0 - 0xF are never pressed.
func (k *Keypad) Key(key byte) bool {
	return key < 16 && k.state&(1<<key) != 0
}

func (k *Keypad) Set(key byte, pressed bool) {
	if key >= 16 {
		return
	}
	if pressed {
		k.state |= 1 << key
	} else {
		k.state &^= 1 << key
	}
}
