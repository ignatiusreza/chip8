package frontend

import (
	"encoding/binary"
	"log"
	"math"
	"time"

	"github.com/ebitengine/oto/v3"
)

const (
	sampleRate = 44100
	beepHz     = 440
	beepVolume = 0.1
)

// Sound is a square-wave beeper that plays while the sound timer is active. It uses
// oto directly rather than Ebitengine's audio package, which only starts once a window
// is open, so it works with the terminal display too.
type Sound struct {
	player *oto.Player // nil when no audio device is available
}

// NewSound opens the audio device, or runs silently when there is none.
func NewSound() *Sound {
	ctx, ready, err := oto.NewContext(&oto.NewContextOptions{
		SampleRate:   sampleRate,
		ChannelCount: 2,
		Format:       oto.FormatFloat32LE,
		BufferSize:   50 * time.Millisecond,
	})
	if err != nil {
		log.Printf("Sound disabled, could not open audio device: %v", err)
		return &Sound{}
	}
	<-ready
	return &Sound{player: ctx.NewPlayer(&squareWave{})}
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
