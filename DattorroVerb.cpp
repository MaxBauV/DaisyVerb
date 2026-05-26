#include "daisy_versio.h"
#include "daisysp.h"
#include "src/VersioReverb.h"

using namespace daisy;
using namespace daisysp;

DaisyVersio  hw;
VersioReverb reverb;

// --- DJ FILTER ---
Svf   dj_filter_l;
Svf   dj_filter_r;
float dj_pot_val = 0.5f; 
float min_freq   = 500.0f;
float max_freq   = 4000.0f;

float out_gain = 0.5f;

constexpr Pin PIN_TOGGLE3_0A = seed::D6;
constexpr Pin PIN_TOGGLE3_0B = seed::D5;
Switch3 hpfSw;

// Desmodus Versio Mapping
constexpr uint8_t BLEND = 0;
constexpr uint8_t SPEED = 1;
constexpr uint8_t TONE  = 2;
constexpr uint8_t INDEX = 3;
constexpr uint8_t REGEN = 4;
constexpr uint8_t SIZE  = 5;
constexpr uint8_t DENSE = 6;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAnalogControls();

	reverb.UpdateParameters(
		{
			hw.GetKnobValue(BLEND),
			hw.GetKnobValue(SIZE),
			hw.GetKnobValue(DENSE),
			hw.GetKnobValue(SPEED),
			hw.GetKnobValue(INDEX),
			hw.GetKnobValue(TONE),
			hpfSw.Read()
		}
	);

	for (size_t i = 0; i < size; i++)
	{
		float sig_out_l = 0.0f;
		float sig_out_r = 0.0f;

		reverb.Process(in[0][i], in[1][i], sig_out_l, sig_out_r);

		out[0][i] = sig_out_l;
		out[1][i] = sig_out_r;
	}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4);
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	reverb.Init(hw.AudioSampleRate());

	hpfSw.Init(PIN_TOGGLE3_0A, PIN_TOGGLE3_0B);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	
	while(1) {
		System::Delay(10);
	}
}