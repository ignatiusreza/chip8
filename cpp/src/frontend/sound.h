#ifndef _CHIP8_FRONTEND_SOUND_H
#define	_CHIP8_FRONTEND_SOUND_H

#include <atomic>
#include <memory>
#include <SDL3/SDL.h>

// Square-wave beeper that plays while the sound timer is active.
class Sound {
  public:
    static constexpr int FREQUENCY = 44100;
    static constexpr int BEEP_HZ = 440;
    static constexpr Sint16 AMPLITUDE = 5000;

  private:
    // read by the audio thread
    std::atomic<bool> _playing{false};
    int _phase = 0;
    // null when no audio device is available
    std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

  public:
    // opens the default audio device, or runs silently when there is none
    Sound();

    void setPlaying(bool playing) { _playing = playing; }

  private:
    // fills the stream with as many samples as the device asks for
    static void callback(void *sound, SDL_AudioStream *stream, int additional, int total);
};

#endif	/* _CHIP8_FRONTEND_SOUND_H */
