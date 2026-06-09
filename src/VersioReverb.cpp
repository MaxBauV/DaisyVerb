#include "VersioReverb.h"
#include "AllpassFilter.h"
#include "hid/switch3.h"

constexpr size_t MAX_ALLPASS_DELAY = 4500U;

VersioReverb::VersioReverb(
	AllpassFilter<4500U> (&series_l)[4],
	AllpassFilter<4500U> (&series_r)[4],
	daisysp::DelayLine<float, DELAYLINE1> &del_0_l,
	daisysp::DelayLine<float, DELAYLINE2> &del_1_l,
	daisysp::DelayLine<float, DELAYLINE3> &del_2_l,
	daisysp::DelayLine<float, DELAYLINE4> &del_3_l,
	daisysp::DelayLine<float, DELAYLINE1> &del_0_r,
	daisysp::DelayLine<float, DELAYLINE2> &del_1_r,
	daisysp::DelayLine<float, DELAYLINE3> &del_2_r,
	daisysp::DelayLine<float, DELAYLINE4> &del_3_r
) :
	series_apf_l_(series_l),
	series_apf_r_(series_r),
	del_0_l_(del_0_l),
	del_1_l_(del_1_l),
	del_2_l_(del_2_l),
	del_3_l_(del_3_l),
	del_0_r_(del_0_r),
	del_1_r_(del_1_r),
	del_2_r_(del_2_r),
	del_3_r_(del_3_r)
{
}

void VersioReverb::Init(float sample_rate)
{
	/** Init Series Allpass Filters */
	unsigned int series_lengths[4] = {223U, 557U, 443U, 337U};

	for(int i = 0; i < 4; i++) {
		series_apf_l_[i].Init(series_lengths[i]);
		series_apf_l_[i].SetGain(0.7f);
		series_apf_r_[i].Init(series_lengths[i]);
		series_apf_r_[i].SetGain(0.7f);
	}

	/** Init Tank Allpass Filters */
	tank_apf_0_l_.Init(1083U); tank_apf_0_l_.SetGain(-0.7f);
	tank_apf_1_l_.Init(2903U); tank_apf_1_l_.SetGain(0.5f);
	tank_apf_2_l_.Init(1464U); tank_apf_2_l_.SetGain(-0.7f);
	tank_apf_3_l_.Init(4283U); tank_apf_3_l_.SetGain(0.5f);

	tank_apf_0_r_.Init(1083U); tank_apf_0_r_.SetGain(-0.7f);
	tank_apf_1_r_.Init(2903U); tank_apf_1_r_.SetGain(0.5f);
	tank_apf_2_r_.Init(1464U); tank_apf_2_r_.SetGain(-0.7f);
	tank_apf_3_r_.Init(4283U); tank_apf_3_r_.SetGain(0.5f);

	/** Init Feedback Delay Lines */
	del_0_l_.Init(); del_0_l_.SetDelay(7182U);
	del_1_l_.Init(); del_1_l_.SetDelay(5999U);
	del_2_l_.Init(); del_2_l_.SetDelay(6801U);
	del_3_l_.Init(); del_3_l_.SetDelay(5101U);

	del_0_r_.Init(); del_0_r_.SetDelay(7182U);
	del_1_r_.Init(); del_1_r_.SetDelay(5999U);
	del_2_r_.Init(); del_2_r_.SetDelay(6801U);
	del_3_r_.Init(); del_3_r_.SetDelay(5101U);

	/** Init Modulation LFO */
	osc_0_l_.Init(sample_rate);
	osc_0_l_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
	osc_0_r_.Init(sample_rate);
	osc_0_r_.SetWaveform(daisysp::Oscillator::WAVE_SIN);

	lpf_0_l_ = lpf_1_l_ = lpf_0_r_ = lpf_1_r_ = 0.0f;

	/** Init Pitchshifter */
	ps_l_.Init(sample_rate);
	ps_r_.Init(sample_rate);

	/** Init End-Of-Chain High & Low Pass Filters */
	hpf_l_.Init(sample_rate);
	hpf_r_.Init(sample_rate);
	lpf_l_.Init(sample_rate);
	lpf_r_.Init(sample_rate);

	hpf_l_.SetDrive(0.0f);
	hpf_r_.SetDrive(0.0f);
	lpf_l_.SetDrive(0.0f);
	lpf_r_.SetDrive(0.0f);

	hpf_l_.SetRes(0.0f);
	hpf_r_.SetRes(0.0f);
	lpf_l_.SetRes(0.0f);
	lpf_r_.SetRes(0.0f);
}

void VersioReverb::UpdateParameters(RtParams params)
{
	currParams_ = params;
}

void VersioReverb::Process(float in_l, float in_r, float &out_wet_l, float &out_wet_r)
{
	float osc_freq = (9.0f * currParams_.oscSpeed) + 1.0f;
	float osc_amp  = (96.0f * currParams_.oscAmp) + 4.0f;

	osc_0_l_.SetFreq(osc_freq);
	osc_0_l_.SetAmp(osc_amp);
	osc_0_r_.SetFreq(osc_freq);
	osc_0_r_.SetAmp(osc_amp);

	float osc_v_l = osc_0_l_.Process();
	float osc_v_r = osc_0_r_.Process();

	/** Read all delays first (prevents phase shifting) */
	float del_3_l_val = del_3_l_.Read();
	float del_1_l_val = del_1_l_.Read();
	float del_0_l_val = del_0_l_.Read();
	float del_2_l_val = del_2_l_.Read();

	float del_3_r_val = del_3_r_.Read();
	float del_1_r_val = del_1_r_.Read();
	float del_0_r_val = del_0_r_.Read();
	float del_2_r_val = del_2_r_.Read();

	/** Calculate the curve (S-type) */
	float x = currParams_.blend;
	float wet_gain = (3.0f * x * x) - (2.0f * x * x * x);

	/** Series Allpass Filters */
	float x_l = in_l * wet_gain; 
	float series_apf_y_l = x_l;
	for(int i = 0; i < 4; i++) {
		series_apf_y_l = series_apf_l_[i].Process(series_apf_y_l);
	}

	float x_r = in_r * wet_gain; 
	float series_apf_y_r = x_r;
	for(int i = 0; i < 4; i++) {
		series_apf_y_r = series_apf_r_[i].Process(series_apf_y_r);
	}

	/** Calculate Delay */
	decay_       = (0.8f * currParams_.size) + 0.1f;
	decay_atten_ = 1.0f - powf(decay_, 2.0f);

	/** Crosstalk (left & right) */
	float feedback_sum_node_0_l = series_apf_y_l + (decay_ * del_3_r_val);
	float feedback_sum_node_1_l = series_apf_y_l + (decay_ * del_1_r_val);

	float feedback_sum_node_0_r = series_apf_y_r + (decay_ * del_3_l_val);
	float feedback_sum_node_1_r = series_apf_y_r + (decay_ * del_1_l_val);

	/** Calculate params for both reverb tanks */
	damping_	= (0.9f * currParams_.damp) + 0.0008f;
	bandwidth_	= 1.0f - damping_;

	/** Left reverb tank */
	float tank_apf_0_out_l = tank_apf_0_l_.ProcessHermite(feedback_sum_node_0_l, 1083.0f + osc_v_l);
	lpf_0_l_               = (del_0_l_val * bandwidth_) + (lpf_0_l_ * damping_);
	del_0_l_.Write(tank_apf_0_out_l);
	float output_node_0_l  = decay_ * lpf_0_l_;

	float tank_apf_1_out_l = tank_apf_1_l_.Process(output_node_0_l);
	del_1_l_.Write(tank_apf_1_out_l);

	float tank_apf_2_out_l = tank_apf_2_l_.ProcessHermite(feedback_sum_node_1_l, 1464.0f + osc_v_l);
	lpf_1_l_               = (del_2_l_val * bandwidth_) + (lpf_1_l_ * damping_);
	del_2_l_.Write(tank_apf_2_out_l);
	float output_node_1_l  = decay_ * lpf_1_l_;

	float tank_apf_3_out_l = tank_apf_3_l_.Process(output_node_1_l);
	del_3_l_.Write(tank_apf_3_out_l);

	/** Right reverb tank */
	float tank_apf_0_out_r = tank_apf_0_r_.ProcessHermite(feedback_sum_node_0_r, 1083.0f + osc_v_r);
	lpf_0_r_               = (del_0_r_val * bandwidth_) + (lpf_0_r_ * damping_);
	del_0_r_.Write(tank_apf_0_out_r);
	float output_node_0_r  = decay_ * lpf_0_r_;

	float tank_apf_1_out_r = tank_apf_1_r_.Process(output_node_0_r);
	del_1_r_.Write(tank_apf_1_out_r);

	float tank_apf_2_out_r = tank_apf_2_r_.ProcessHermite(feedback_sum_node_1_r, 1464.0f + osc_v_r);
	lpf_1_r_               = (del_2_r_val * bandwidth_) + (lpf_1_r_ * damping_);
	del_2_r_.Write(tank_apf_2_out_r);
	float output_node_1_r  = decay_ * lpf_1_r_;

	float tank_apf_3_out_r = tank_apf_3_r_.Process(output_node_1_r);
	del_3_r_.Write(tank_apf_3_out_r);

	/** Mix Output Taps */
	out_wet_l = tank_apf_0_out_l - output_node_0_l + tank_apf_1_out_l - feedback_sum_node_0_l;
	out_wet_l *= (decay_atten_ * 1.5f * 1.5f);

	out_wet_r = tank_apf_2_out_r - output_node_1_r + tank_apf_3_out_r - feedback_sum_node_1_r;
	out_wet_r *= (decay_atten_ * 1.5f * 1.5f);

	/** Pitch Shifter */
	float transposeValue;
	switch (currParams_.psRange)
	{
		case daisy::Switch3::POS_LEFT:
			transposeValue = daisysp::fmap(currParams_.psValue, -24.0f, 24.0f, daisysp::Mapping::LINEAR);
			ps_l_.SetTransposition(transposeValue);
			ps_r_.SetTransposition(transposeValue);
			out_wet_l = ps_l_.Process(out_wet_l);
			out_wet_r = ps_r_.Process(out_wet_r);
			break;
		case daisy::Switch3::POS_CENTER:
			break;
		case daisy::Switch3::POS_RIGHT:
			transposeValue = daisysp::fmap(currParams_.psValue, -12.0f, 12.0f, daisysp::Mapping::LINEAR);
			ps_l_.SetTransposition(transposeValue);
			ps_r_.SetTransposition(transposeValue);
			out_wet_l = ps_l_.Process(out_wet_l);
			out_wet_r = ps_r_.Process(out_wet_r);
			break;
	}
	

	/** High Pass filter on wet signal */
	switch (currParams_.hpf)
	{
		case daisy::Switch3::POS_LEFT:
			break;
		case daisy::Switch3::POS_CENTER:
			hpf_l_.SetFreq(HighPassFreq::LOWS);
			hpf_r_.SetFreq(HighPassFreq::LOWS);
			hpf_l_.Process(out_wet_l);
			hpf_r_.Process(out_wet_r);
			out_wet_l = hpf_l_.High();
			out_wet_r = hpf_r_.High();
			break;
		case daisy::Switch3::POS_RIGHT:
			hpf_l_.SetFreq(HighPassFreq::LOWS);
			hpf_r_.SetFreq(HighPassFreq::LOWS);
			hpf_l_.Process(out_wet_l);
			hpf_r_.Process(out_wet_r);
			out_wet_l = hpf_l_.High();
			out_wet_r = hpf_r_.High();
			break;
	}

	/** Low Pass filter on wet signal */
	float lpf_freq = daisysp::fmap(currParams_.lpf, 20.0f, 20000.0f, daisysp::Mapping::LINEAR);
	lpf_l_.SetFreq(lpf_freq);
	lpf_r_.SetFreq(lpf_freq);
	lpf_l_.Process(out_wet_l);
	lpf_r_.Process(out_wet_r);
	out_wet_l = lpf_l_.Low();
	out_wet_r = lpf_r_.Low();

	/** Blend Mixing */
	float dry_gain = 1.0f - (x * x * x);

	out_wet_l = out_wet_l + (in_l * dry_gain);
	out_wet_r = out_wet_r + (in_r * dry_gain);
}
