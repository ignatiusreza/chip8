#ifndef _CHIP8_SOUND_H
#define	_CHIP8_SOUND_H

#include <cmath>
#include "SDL.h"

void callback(void *_sound, Uint8 *_stream, int _length);

class Sound {
  public:
    static const int FREQUENCY = 44100;

  private:
    int _durationLeft;

  public:
    Sound() {
      SDL_AudioSpec desiredSpec;
      SDL_AudioSpec obtainedSpec;

      _durationLeft = 0;

      desiredSpec.freq = FREQUENCY;
      desiredSpec.format = AUDIO_S16SYS;
      desiredSpec.channels = 1;
      desiredSpec.samples = 2048;
      desiredSpec.callback = callback;
      desiredSpec.userdata = this;

      SDL_OpenAudio(&desiredSpec, &obtainedSpec);

      // start play audio
      SDL_PauseAudio(0);
    }

    ~Sound() {
      SDL_CloseAudio();
    }

    void beep(int duration) {
      _durationLeft = duration * FREQUENCY / 1000;
    }

    friend void callback(void *_sound, Uint8 *_stream, int _length);
};

void callback(void *_sound, Uint8 *_stream, int _length) {
  Sound  *sound  = static_cast<Sound *>(_sound);
  Sint16 *stream = reinterpret_cast<Sint16*>(_stream);
  int length = _length / 2;
  int sampleLength = std::min(length,sound->_durationLeft);
  int i = 0;

  sound->_durationLeft -= sampleLength;

  for(;i < sampleLength; i++) stream[i] = 20000;
  for(;i < length      ; i++) stream[i] = 0;
}

#endif	/* _CHIP8_SOUND_H */
