#ifndef _CHIP8_SOUND_H
#define	_CHIP8_SOUND_H

#include "SDL.h"

void callback(void *_sound, Uint8 *_stream, int _length);

// Square-wave beeper that plays while the sound timer is active.
class Sound {
  public:
    static const int FREQUENCY = 44100;
    static const int BEEP_HZ = 440;
    static const Sint16 AMPLITUDE = 5000;

  private:
    bool _playing;
    int _phase;

  public:
    Sound() {
      SDL_AudioSpec desiredSpec;
      SDL_AudioSpec obtainedSpec;

      _playing = false;
      _phase = 0;

      desiredSpec.freq = FREQUENCY;
      desiredSpec.format = AUDIO_S16SYS;
      desiredSpec.channels = 1;
      // small buffer so the beep starts and stops close to the timer
      desiredSpec.samples = 512;
      desiredSpec.callback = callback;
      desiredSpec.userdata = this;

      SDL_OpenAudio(&desiredSpec, &obtainedSpec);

      // start play audio
      SDL_PauseAudio(0);
    }

    ~Sound() {
      SDL_CloseAudio();
    }

    void setPlaying(bool playing) {
      SDL_LockAudio();
      _playing = playing;
      SDL_UnlockAudio();
    }

    friend void callback(void *_sound, Uint8 *_stream, int _length);
};

void callback(void *_sound, Uint8 *_stream, int _length) {
  Sound  *sound  = static_cast<Sound *>(_sound);
  Sint16 *stream = reinterpret_cast<Sint16*>(_stream);
  int length = _length / 2;
  int halfPeriod = Sound::FREQUENCY / Sound::BEEP_HZ / 2;

  for(int i = 0; i < length; i++) {
    if(sound->_playing) {
      stream[i] = (sound->_phase / halfPeriod) % 2 ? -Sound::AMPLITUDE : Sound::AMPLITUDE;
      sound->_phase = (sound->_phase + 1) % (halfPeriod * 2);
    } else {
      stream[i] = 0;
    }
  }
}

#endif	/* _CHIP8_SOUND_H */
