#include "sound.h"

#include <iostream>
#include <vector>

Sound::Sound() : _stream(nullptr, SDL_DestroyAudioStream) {
  if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    std::cerr << "Sound disabled, could not open audio device: " << SDL_GetError() << std::endl;
    return;
  }

  const SDL_AudioSpec spec = {SDL_AUDIO_S16, 1, FREQUENCY};
  _stream.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this));
  if(!_stream) {
    std::cerr << "Sound disabled, could not open audio device: " << SDL_GetError() << std::endl;
    return;
  }

  // start play audio; the stream starts paused
  SDL_ResumeAudioStreamDevice(_stream.get());
}

void Sound::callback(void *_sound, SDL_AudioStream *stream, int additional, int) {
  Sound *sound = static_cast<Sound *>(_sound);
  const int halfPeriod = FREQUENCY / BEEP_HZ / 2;
  const bool playing = sound->_playing;

  std::vector<Sint16> samples(additional / sizeof(Sint16));
  for(Sint16 &sample : samples) {
    if(playing) {
      sample = (sound->_phase / halfPeriod) % 2 ? -AMPLITUDE : AMPLITUDE;
      sound->_phase = (sound->_phase + 1) % (halfPeriod * 2);
    } else {
      sample = 0;
    }
  }
  SDL_PutAudioStreamData(stream, samples.data(), samples.size() * sizeof(Sint16));
}
