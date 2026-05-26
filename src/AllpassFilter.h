#pragma once
#include "daisysp.h"

class AllpassFilter 
{
public:

	/**
	 * @brief Initializes the all-pass filter with a specific delay size.
	 * @param delay_samples The initial delay time measured in audio samples.
	 */
	void Init(unsigned int delay_samples) ;

	/**
	 * @brief Updates the filter's delay time dynamically.
	 * @param delay_samples The new delay time in samples.
	 */
	void SetDelay(unsigned int delay_samples);

	/**
	 * @brief Sets the feedback/feedforward gain coefficient.
	 * @param gain The gain factor (typically between -1.0f and 1.0f).
	 */
	void SetGain(float gain);

	/**
	 * @brief Processes a single audio sample through the standard all-pass filter.
	 * @param input The incoming audio sample.
	 * @return The filtered output audio sample.
	 */
	float Process(float input);

	/**
	 * @brief Processes a single audio sample using Hermite interpolation for dynamic delay times.
	 * @param input The incoming audio sample.
	 * @param delay_mod The modulated delay time (supports fractional values for smooth modulation).
	 * @return The filtered output audio sample.
	 */
	float ProcessHermite(float input, float delay_mod);

private:
	daisysp::DelayLine<float, 4500U> delay_line_; 
	float gain_;
};
