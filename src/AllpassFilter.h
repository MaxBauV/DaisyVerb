#pragma once
#include "daisysp.h"

template <size_t MAX_SIZE = 4500U>
class AllpassFilter 
{
public:

	/**
	 * @brief Initializes the all-pass filter with a specific delay size.
	 * @param delay_samples The initial delay time measured in audio samples.
	 */
	void Init(unsigned int delay_samples)
	{
		delay_line_.Init();
		delay_line_.SetDelay(delay_samples);
		gain_ = 0.7f;
	}

	/**
	 * @brief Updates the filter's delay time dynamically.
	 * @param delay_samples The new delay time in samples.
	 */
	void SetDelay(unsigned int delay_samples)
	{
		delay_line_.SetDelay(delay_samples);
	}

	/**
	 * @brief Sets the feedback/feedforward gain coefficient.
	 * @param gain The gain factor (typically between -1.0f and 1.0f).
	 */
	void SetGain(float gain)
	{
		gain_ = gain;
	}

	/**
	 * @brief Processes a single audio sample through the standard all-pass filter.
	 * @param input The incoming audio sample.
	 * @return The filtered output audio sample.
	 */
	float Process(float input)
	{
		float del_out = delay_line_.Read();
		float w       = gain_ * (del_out - input);
		delay_line_.Write(w + input);
		return w + del_out;
	}

	/**
	 * @brief Processes a single audio sample using Hermite interpolation for dynamic delay times.
	 * @param input The incoming audio sample.
	 * @param delay_mod The modulated delay time (supports fractional values for smooth modulation).
	 * @return The filtered output audio sample.
	 */
	float ProcessHermite(float input, float delay_mod)
	{
		float del_out = delay_line_.ReadHermite(delay_mod);
		float w       = gain_ * (del_out - input);
		delay_line_.Write(w + input);
		return w + del_out;
	}

private:
	daisysp::DelayLine<float, MAX_SIZE> delay_line_; 
	float gain_;
};
