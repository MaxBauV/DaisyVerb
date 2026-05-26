#include "AllpassFilter.h"

void AllpassFilter::Init(unsigned int delay_samples)
{
	delay_line_.Init();
	delay_line_.SetDelay(delay_samples);
	gain_ = 0.7f;
}

void AllpassFilter::SetDelay(unsigned int delay_samples)
{
	delay_line_.SetDelay(delay_samples);
}

void AllpassFilter::SetGain(float gain)
{
	gain_ = gain;
}

float AllpassFilter::Process(float input)
{
	float del_out = delay_line_.Read();
	float w       = gain_ * (del_out - input);
	delay_line_.Write(w + input);
	return w + del_out;
}

float AllpassFilter::ProcessHermite(float input, float delay_mod)
{
	float del_out = delay_line_.ReadHermite(delay_mod);
	float w       = gain_ * (del_out - input);
	delay_line_.Write(w + input);
	return w + del_out;
}
