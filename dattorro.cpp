#include "daisy_versio.h"
#include "daisysp.h"

#define SERIES_APF_NUM 4U

using namespace daisy;
using namespace daisysp;

DaisyVersio hw;

// --- DJ FILTER ERWEITERUNG START ---
Svf dj_filter_l;
Svf dj_filter_r;
float dj_pot_val = 0.5f; // Steuerung über Knob 5
float min_freq = 500.0f;
float max_freq = 4000.0f;
// --- DJ FILTER ERWEITERUNG ENDE ---

static DelayLine<float, 557U  + 1U> series_apf[SERIES_APF_NUM];

static DelayLine<float, 1083U + 101U> tank_apf_0;
static DelayLine<float, 2903U + 1U> tank_apf_1;
static DelayLine<float, 1464U + 101U> tank_apf_2;
static DelayLine<float, 4283U + 1U> tank_apf_3;

static DelayLine<float, 7182U + 1U> del_0;
static DelayLine<float, 5999U + 1U> del_1;
static DelayLine<float, 6801U + 1U> del_2;
static DelayLine<float, 5101U + 1U> del_3;

static Oscillator osc_0;

float sig_out[2];
unsigned char idx;
float x;
float osc_v;
float series_apf_y, apf_del_out, apf_w;
float feedback_sum_node_0, feedback_sum_node_1;
float tank_apf_0_out, tank_apf_1_out, tank_apf_2_out, tank_apf_3_out;
float output_node_0, output_node_1;
float lpf_0, lpf_1 = 0.0f;

float damping = 0.0008f;
float bandwidth = 1.0f - damping;
float decay = 0.5f;
float decay_atten = 0.5f;

float out_gain = 0.5;

float osc_amp;
float osc_freq;

// Desmodus Versio Mapping
constexpr uint8_t BLEND = 0;
constexpr uint8_t SPEED = 1;
constexpr uint8_t TONE = 2;
constexpr uint8_t INDEX = 3;
constexpr uint8_t REGEN = 4;
constexpr uint8_t SIZE = 5;
constexpr uint8_t DENSE = 6;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAnalogControls(); // Normalize CV inputs
	out_gain = hw.GetKnobValue(BLEND);

	damping = (0.9f * hw.GetKnobValue(DENSE)) + 0.0008f;
	bandwidth = 1.0f - damping;
	
	decay = (0.8f * hw.GetKnobValue(SIZE)) + 0.1f;
	decay_atten = 1.0f - powf(decay, 2.f);
	
	osc_freq = (9.0f * hw.GetKnobValue(SPEED)) + 1.0f;
	osc_amp  = (96.0f * hw.GetKnobValue(INDEX)) + 4.0f;
	osc_0.SetFreq(osc_freq);
	osc_0.SetAmp(osc_amp);

	float drive_val = hw.GetKnobValue(REGEN) * 0.01f;
	dj_filter_l.SetDrive(drive_val);
	dj_filter_r.SetDrive(drive_val);

	// Poti-Wert für den DJ-Filter von Knob 5 abfragen
	dj_pot_val = hw.GetKnobValue(TONE); 

	// Filter-Parameter einmal pro Audio-Block berechnen (spart CPU)
	float filter_freq = max_freq;
	int filter_mode = 0; // 0 = Bypass, 1 = Lowpass, 2 = Highpass

	if (dj_pot_val < 0.48f) 
	{
		filter_mode = 1; // Lowpass
		float norm = dj_pot_val / 0.48f;
		filter_freq = min_freq + (max_freq - min_freq) * (norm * norm);
	}
	else if (dj_pot_val > 0.52f) 
	{
		filter_mode = 2; // Highpass
		float norm = (dj_pot_val - 0.52f) / 0.48f;
		filter_freq = min_freq + (max_freq - min_freq) * (norm * norm);
	}

	dj_filter_l.SetFreq(filter_freq);
	dj_filter_r.SetFreq(filter_freq);

	for (size_t i = 0; i < size; i++)
	{
		x = 0.5f * (in[0][i] + in[1][i]) * out_gain; // Mono
		osc_v = osc_0.Process();

		// Series APFs
		series_apf_y = x;
		for(idx = 0; idx < SERIES_APF_NUM; idx++) {
			// One-Multiply APF
			apf_del_out = series_apf[idx].Read();
			apf_w = 0.7f * (apf_del_out - series_apf_y);
			series_apf[idx].Write(apf_w + series_apf_y);
			series_apf_y = apf_w + apf_del_out;
		}

		feedback_sum_node_0 = series_apf_y + (decay * del_3.Read());
		feedback_sum_node_1 = series_apf_y + (decay * del_1.Read());

		// Tank APF 0
		apf_del_out = tank_apf_0.ReadHermite(1083.0f + osc_v);
		apf_w = -0.7f * (apf_del_out - feedback_sum_node_0);
		tank_apf_0.Write(apf_w + feedback_sum_node_0);
		tank_apf_0_out = apf_w + apf_del_out;

		lpf_0 = (del_0.Read() * bandwidth) + (lpf_0 * damping);
		del_0.Write(tank_apf_0_out);

		output_node_0 = decay * lpf_0;

		// Tank APF 1
		apf_del_out = tank_apf_1.Read();
		apf_w = 0.5f * (apf_del_out - output_node_0);
		tank_apf_1.Write(apf_w + output_node_0);
		tank_apf_1_out = apf_w + apf_del_out;

		del_1.Write(tank_apf_1_out);

		// Tank APF 2
		apf_del_out = tank_apf_2.ReadHermite(1464.0f + osc_v);
		apf_w = -0.7f * (apf_del_out - feedback_sum_node_1);
		tank_apf_2.Write(apf_w + feedback_sum_node_1);
		tank_apf_2_out = apf_w + apf_del_out;

		lpf_1 = (del_2.Read() * bandwidth) + (lpf_1 * damping);
		del_2.Write(tank_apf_2_out);

		output_node_1 = decay * lpf_1;

		// Tank APF 3
		apf_del_out = tank_apf_3.Read();
		apf_w = 0.5f * (apf_del_out - output_node_1);
		tank_apf_3.Write(apf_w + output_node_1);
		tank_apf_3_out = apf_w + apf_del_out;

		del_3.Write(tank_apf_3_out);

		// Tap outputs (Hier entsteht das reine WET-Signal)
		sig_out[0] = tank_apf_0_out - output_node_0 + tank_apf_1_out - feedback_sum_node_0;
		sig_out[1] = tank_apf_2_out - output_node_1 + tank_apf_3_out - feedback_sum_node_1;

		sig_out[0] *= (decay_atten * 1.5f);
		sig_out[1] *= (decay_atten * 1.5f);

		sig_out[0] *= 1.5;
		sig_out[1] *= 1.5;

		// --- DJ FILTER AUF WET-SIGNAL ANWENDEN ---
		if (filter_mode != 0)
		{
			// Linker Kanal (Wet)
			dj_filter_l.Process(sig_out[0]);
			sig_out[0] = (filter_mode == 1) ? dj_filter_l.Low() : dj_filter_l.High();

			// Rechter Kanal (Wet)
			dj_filter_r.Process(sig_out[1]);
			sig_out[1] = (filter_mode == 1) ? dj_filter_r.Low() : dj_filter_r.High();
		}
		// -----------------------------------------

		// Wet + Dry Mix am Ausgang
		out[0][i] = sig_out[0] + ((1.0f - out_gain) * in[0][i]);
		out[1][i] = sig_out[1] + ((1.0f - out_gain) * in[1][i]);
	}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4);
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	
	// Initialize & Set Series APF Delay Lines
	unsigned short series_apf_length[4] = {223U, 557U, 443U, 337U};
	for(idx = 0; idx < SERIES_APF_NUM; idx++){
		series_apf[idx].Init();
		series_apf[idx].SetDelay((unsigned int) series_apf_length[idx]);
	}

	// Initialize & Set Tank APF Delay Lines
	tank_apf_0.Init();
	tank_apf_0.SetDelay(1083U);

	tank_apf_1.Init();
	tank_apf_1.SetDelay(2903U);

	tank_apf_2.Init();
	tank_apf_2.SetDelay(1464U);

	tank_apf_3.Init();
	tank_apf_3.SetDelay(4283U);

	// Initialize & Set Tank Delay Lines
	del_0.Init();
	del_0.SetDelay(7182U);

	del_1.Init();
	del_1.SetDelay(5999U);

	del_2.Init();
	del_2.SetDelay(6801U);

	del_3.Init();
	del_3.SetDelay(5101U);

	// Initialize & Set Oscillator
	osc_0.Init(hw.AudioSampleRate());
	osc_0.SetWaveform(osc_0.WAVE_SIN);
	osc_0.SetFreq(1);
	osc_0.SetAmp(4.0f);

	// --- DJ FILTER INITIALISIERUNG ---
	dj_filter_l.Init(hw.AudioSampleRate());
	dj_filter_r.Init(hw.AudioSampleRate());
	dj_filter_l.SetRes(0.05f); // Angenehme Resonanz für Hall-Fahnen
	dj_filter_r.SetRes(0.05f);
	// ---------------------------------

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	
	while(1) {
		
	}
}