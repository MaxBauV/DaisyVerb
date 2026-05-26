#include "VersioReverb.h"

void VersioReverb::Init(float sample_rate)
{
	unsigned int series_lengths[4] = {223U, 557U, 443U, 337U};

	// Linker und rechter Kanal parallel initialisieren
	for(int i = 0; i < 4; i++) {
		series_apf_l_[i].Init(series_lengths[i]);
		series_apf_l_[i].SetGain(0.7f);
		series_apf_r_[i].Init(series_lengths[i]);
		series_apf_r_[i].SetGain(0.7f);
	}

	// Tank-APFs für beide Seiten einrichten
	tank_apf_0_l_.Init(1083U); tank_apf_0_l_.SetGain(-0.7f);
	tank_apf_1_l_.Init(2903U); tank_apf_1_l_.SetGain(0.5f);
	tank_apf_2_l_.Init(1464U); tank_apf_2_l_.SetGain(-0.7f);
	tank_apf_3_l_.Init(4283U); tank_apf_3_l_.SetGain(0.5f);

	tank_apf_0_r_.Init(1083U); tank_apf_0_r_.SetGain(-0.7f);
	tank_apf_1_r_.Init(2903U); tank_apf_1_r_.SetGain(0.5f);
	tank_apf_2_r_.Init(1464U); tank_apf_2_r_.SetGain(-0.7f);
	tank_apf_3_r_.Init(4283U); tank_apf_3_r_.SetGain(0.5f);

	// Feedback-Delay-Lines
	del_0_l_.Init(); del_0_l_.SetDelay(7182U);
	del_1_l_.Init(); del_1_l_.SetDelay(5999U);
	del_2_l_.Init(); del_2_l_.SetDelay(6801U);
	del_3_l_.Init(); del_3_l_.SetDelay(5101U);

	del_0_r_.Init(); del_0_r_.SetDelay(7182U);
	del_1_r_.Init(); del_1_r_.SetDelay(5999U);
	del_2_r_.Init(); del_2_r_.SetDelay(6801U);
	del_3_r_.Init(); del_3_r_.SetDelay(5101U);

	// Modulations-Oszillatoren
	osc_0_l_.Init(sample_rate);
	osc_0_l_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
	osc_0_r_.Init(sample_rate);
	osc_0_r_.SetWaveform(daisysp::Oscillator::WAVE_SIN);

	lpf_0_l_ = lpf_1_l_ = lpf_0_r_ = lpf_1_r_ = 0.0f;

	// End-of-Chain LPF & HPF on reverb wet
	hpf_.Init(sample_rate);
	lpf_.Init(sample_rate);
}

void VersioReverb::UpdateParameters(RtParams params)
{
	currParams_ = params;

	damping_     = (0.9f * params.damp) + 0.0008f;
	bandwidth_   = 1.0f - damping_;
	decay_       = (0.8f * params.size) + 0.1f;
	decay_atten_ = 1.0f - powf(decay_, 2.0f);

	float osc_freq = (9.0f * params.oscSpeed) + 1.0f;
	float osc_amp  = (96.0f * params.oscAmp) + 4.0f;

	osc_0_l_.SetFreq(osc_freq);
	osc_0_l_.SetAmp(osc_amp);
	osc_0_r_.SetFreq(osc_freq);
	osc_0_r_.SetAmp(osc_amp);
}

void VersioReverb::Process(float in_l, float in_r, float &out_wet_l, float &out_wet_r)
{
	float osc_v_l = osc_0_l_.Process();
	float osc_v_r = osc_0_r_.Process();

	// 1. SCHRITT: ALLE DELAYS ZUERST LESEN (Verhindert Phasen-Asymmetrie)
	float del_3_l_val = del_3_l_.Read();
	float del_1_l_val = del_1_l_.Read();
	float del_0_l_val = del_0_l_.Read();
	float del_2_l_val = del_2_l_.Read();

	float del_3_r_val = del_3_r_.Read();
	float del_1_r_val = del_1_r_.Read();
	float del_0_r_val = del_0_r_.Read();
	float del_2_r_val = del_2_r_.Read();

	// 2. SCHRITT: INPUT & SERIELLE APFs (Bleiben getrennt)
	float x_l = in_l * currParams_.blend; 
	float series_apf_y_l = x_l;
	for(int i = 0; i < 4; i++) {
		series_apf_y_l = series_apf_l_[i].Process(series_apf_y_l);
	}

	float x_r = in_r * currParams_.blend; 
	float series_apf_y_r = x_r;
	for(int i = 0; i < 4; i++) {
		series_apf_y_r = series_apf_r_[i].Process(series_apf_y_r);
	}

	// 3. SCHRITT: CROSSTALK UND REVERB TANK BERECHNEN
	// Links nutzt das Feedback von RECHTS, Rechts nutzt das Feedback von LINKS
	float feedback_sum_node_0_l = series_apf_y_l + (decay_ * del_3_r_val); // <-- Überkreuz
	float feedback_sum_node_1_l = series_apf_y_l + (decay_ * del_1_r_val); // <-- Überkreuz

	float feedback_sum_node_0_r = series_apf_y_r + (decay_ * del_3_l_val); // <-- Überkreuz
	float feedback_sum_node_1_r = series_apf_y_r + (decay_ * del_1_l_val); // <-- Überkreuz

	// --- LINKER KANAL TANK ---
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

	// --- RECHTER KANAL TANK ---
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

	// 4. SCHRITT: TAP OUTPUTS MISCHEN
	out_wet_l = tank_apf_0_out_l - output_node_0_l + tank_apf_1_out_l - feedback_sum_node_0_l;
	out_wet_l *= (decay_atten_ * 1.5f * 1.5f);

	out_wet_r = tank_apf_2_out_r - output_node_1_r + tank_apf_3_out_r - feedback_sum_node_1_r;
	out_wet_r *= (decay_atten_ * 1.5f * 1.5f);

	// Filtering

	// High-Pass Filter
	// switch (hpfSt)

	// Mix with dry signal
	out_wet_l = out_wet_l + ((1.0 - currParams_.blend) * in_l);
	out_wet_r = out_wet_r + ((1.0 - currParams_.blend) * in_r);
}
