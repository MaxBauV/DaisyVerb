#pragma once
#include "daisysp.h"

class AllpassFilter 
{
public:
    void Init(unsigned int delay_samples) 
    {
        delay_line_.Init();
        delay_line_.SetDelay(delay_samples);
        gain_ = 0.7f; // Standard-Gain, wird im Reverb überschrieben
    }

    void SetDelay(unsigned int delay_samples) 
    {
        delay_line_.SetDelay(delay_samples);
    }

    inline void SetGain(float gain) { gain_ = gain; }

    inline float Process(float input) 
    {
        float del_out = delay_line_.Read();
        float w       = gain_ * (del_out - input);
        delay_line_.Write(w + input);
        return w + del_out;
    }

    inline float ProcessHermite(float input, float delay_mod) 
    {
        float del_out = delay_line_.ReadHermite(delay_mod);
        float w       = gain_ * (del_out - input);
        delay_line_.Write(w + input);
        return w + del_out;
    }

private:
    // Puffergröße dynamisch groß genug für das größte Allpass-Delay (4283 + Modulation)
    daisysp::DelayLine<float, 4500U> delay_line_; 
    float gain_;
};
