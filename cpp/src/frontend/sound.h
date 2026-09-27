#ifndef _CHIP8_FRONTEND_SOUND_H
#define	_CHIP8_FRONTEND_SOUND_H

#include "SDL.h"

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
    Sound();
    ~Sound();

    void setPlaying(bool playing);

  private:
    static void callback(void *sound, Uint8 *stream, int length);
};

#endif	/* _CHIP8_FRONTEND_SOUND_H */
