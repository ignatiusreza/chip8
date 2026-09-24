use std::num::NonZero;

use rodio::source::{Function, SignalGenerator, Source};
use rodio::{DeviceSinkBuilder, MixerDeviceSink, Player};

const SAMPLE_RATE: u32 = 44100;
const BEEP_HZ: f32 = 440.0;

/// Square-wave beeper that plays while the sound timer is active.
pub struct Sound {
    // the device sink must stay alive for the player to keep producing sound;
    // None when no audio device is available
    output: Option<(MixerDeviceSink, Player)>,
}

impl Sound {
    /// Opens the default audio device, or runs silently when there is none.
    pub fn new() -> Self {
        let output = Self::open()
            .inspect_err(|e| eprintln!("Sound disabled, could not open audio device: {e}"))
            .ok();
        Sound { output }
    }

    fn open() -> Result<(MixerDeviceSink, Player), rodio::DeviceSinkError> {
        let sink = DeviceSinkBuilder::open_default_sink()?;
        let player = Player::connect_new(sink.mixer());
        let sample_rate = NonZero::new(SAMPLE_RATE).unwrap();
        player.append(SignalGenerator::new(sample_rate, BEEP_HZ, Function::Square).amplify(0.1));
        player.pause();
        Ok((sink, player))
    }

    pub fn set_playing(&self, playing: bool) {
        if let Some((_, player)) = &self.output {
            if playing {
                player.play();
            } else {
                player.pause();
            }
        }
    }
}
