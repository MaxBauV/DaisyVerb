#include "daisy_versio.h"
#include "daisysp.h"
#include "src/VersioReverb.h"

using namespace daisy;
using namespace daisysp;

/** Memory-intensive instances are stored in SDRAM and passed by reference to the reverb class. */
PitchShifter DSY_SDRAM_BSS ps_l;
PitchShifter DSY_SDRAM_BSS ps_r;

DelayLine<float, DELAYLINE1> DSY_SDRAM_BSS del_0_l;
DelayLine<float, DELAYLINE2> DSY_SDRAM_BSS del_1_l;
DelayLine<float, DELAYLINE3> DSY_SDRAM_BSS del_2_l;
DelayLine<float, DELAYLINE4> DSY_SDRAM_BSS del_3_l;

DelayLine<float, DELAYLINE1> DSY_SDRAM_BSS del_0_r;
DelayLine<float, DELAYLINE2> DSY_SDRAM_BSS del_1_r;
DelayLine<float, DELAYLINE3> DSY_SDRAM_BSS del_2_r;
DelayLine<float, DELAYLINE4> DSY_SDRAM_BSS del_3_r;

DaisyVersio  hw;
VersioReverb reverb(ps_l, ps_r, del_0_l, del_1_l, del_2_l, del_3_l, del_0_r, del_1_r, del_2_r, del_3_r);

/** Pins & instances of the 3-way switches */
constexpr Pin PIN_TOGGLE3_0A = seed::D6;
constexpr Pin PIN_TOGGLE3_0B = seed::D5;
constexpr Pin PIN_TOGGLE3_1A = seed::D1;
constexpr Pin PIN_TOGGLE3_1B = seed::D0;

Switch3 hpfSw;
Switch3 psRangeSw;

/** Using Desmodus Versio Names for Pot-Mapping */
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
			hpfSw.Read(),
			hw.GetKnobValue(REGEN),
			psRangeSw.Read()
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
	psRangeSw.Init(PIN_TOGGLE3_1A, PIN_TOGGLE3_1B);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	
	while(1) {
		System::Delay(10);
	}
}