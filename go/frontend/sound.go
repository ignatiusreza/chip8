package frontend

import (
	"encoding/binary"
	"log"
	"math"
	"time"

	"github.com/hajimehoshi/ebiten/v2/audio"
)

const (
	sampleRate = 44100
	beepHz     = 440
	beepVolume = 0.1
)

// Sound is a square-wave beeper that plays while the sound timer is active.
type Sound struct {
	player *audio.Player // nil when no audio device is available
}

// NewSound opens the audio device, or runs silently when there is none.
func NewSound() *Sound {
	player, err := audio.NewContext(sampleRate).NewPlayerF32(&squareWave{})
	if err != nil {
		log.Printf("Sound disabled, could not open audio device: %v", err)
		return &Sound{}
	}
	player.SetBufferSize(50 * time.Millisecond)
	return &Sound{player: player}
}

func (s *Sound) SetPlaying(playing bool) {
	if s.player == nil || playing == s.player.IsPlaying() {
		return
	}
	if playing {
		s.player.Play()
	} else {
		s.player.Pause()
	}
}

// squareWave is an endless 440Hz square wave, as 32-bit float stereo PCM.
type squareWave struct {
	pos int
}

func (s *squareWave) Read(buf []byte) (int, error) {
	const halfPeriod = sampleRate / beepHz / 2
	const frameSize = 8 // 2 channels * 4 bytes
	n := len(buf) / frameSize * frameSize
	for i := 0; i < n; i += frameSize {
		sample := float32(beepVolume)
		if (s.pos/halfPeriod)%2 == 1 {
			sample = -sample
		}
		bits := math.Float32bits(sample)
		binary.LittleEndian.PutUint32(buf[i:], bits)
		binary.LittleEndian.PutUint32(buf[i+4:], bits)
		s.pos = (s.pos + 1) % (halfPeriod * 2)
	}
	return n, nil
}
