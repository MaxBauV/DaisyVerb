#pragma once
#include "daisysp.h"
#include "AllpassFilter.h"

/**
 * @brief Cutoff frequencies for the internal high-pass filter.
 * Defines preset frequency values of the end-of-chain hpf.
 */
enum HighPassFreq
{
	LOWS = 400,
	MIDS = 1000,
};

/**
 * @brief Configuration parameters for the VersioReverb DSP engine.
 * This structure holds all real-time controllable parameters for the reverberation algorithm.
 */
struct RtParams
{
	float blend;
	float size;
	float damp;
	float oscSpeed;
	float oscAmp;
	float lpf;
	int hpf;
	float psValue;
	int psRange;
};

class VersioReverb 
{
public:

	VersioReverb(daisysp::PitchShifter &ps_l, daisysp::PitchShifter &ps_r);
	/** 
	 * @brief Initializes the reverb effect with the system sample rate.
	 * @param sample_rate The sampling rate of the audio engine in Hz
	 */
	void Init(float sample_rate);

	/**
	 * @brief Updates the internal DSP parameters.
	 * @param params Struct containing the new parameter values.
	 */
	void UpdateParameters(RtParams params);

	/**
	 * @brief Processes a stereo block or sample of audio.
	 * @param in_l Left input channel sample.
	 * @param in_r Right input channel sample.
	 * @param[out] out_wet_l Reference to store the processed left wet signal.
	 * @param[out] out_wet_r Reference to store the processed right wet signal.
	 */
	void Process(float in_l, float in_r, float &out_wet_l, float &out_wet_r);

private:
	AllpassFilter series_apf_l_[4];
	AllpassFilter series_apf_r_[4];
	
	AllpassFilter tank_apf_0_l_, tank_apf_1_l_, tank_apf_2_l_, tank_apf_3_l_;
	AllpassFilter tank_apf_0_r_, tank_apf_1_r_, tank_apf_2_r_, tank_apf_3_r_;

	daisysp::DelayLine<float, 7183U> del_0_l_;
	daisysp::DelayLine<float, 6000U> del_1_l_;
	daisysp::DelayLine<float, 6802U> del_2_l_;
	daisysp::DelayLine<float, 5102U> del_3_l_;

	daisysp::DelayLine<float, 7183U> del_0_r_;
	daisysp::DelayLine<float, 6000U> del_1_r_;
	daisysp::DelayLine<float, 6802U> del_2_r_;
	daisysp::DelayLine<float, 5102U> del_3_r_;

	daisysp::Oscillator osc_0_l_;
	daisysp::Oscillator osc_0_r_;

	daisysp::Svf hpf_l_;
	daisysp::Svf hpf_r_;
	daisysp::Svf lpf_l_;
	daisysp::Svf lpf_r_;

	daisysp::PitchShifter &ps_l_;
	daisysp::PitchShifter &ps_r_;

	RtParams currParams_;

	float lpf_0_l_, lpf_1_l_, lpf_0_r_, lpf_1_r_;
	float damping_, bandwidth_, decay_, decay_atten_;
};
