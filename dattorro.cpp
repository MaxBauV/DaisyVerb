#include "daisy_versio.h"
#include "daisysp.h"
#include "VersioReverb.h"

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
    out_gain = hw.GetKnobValue(BLEND);

    // Reverb Parameter Updaten
    reverb.UpdateParameters(
        hw.GetKnobValue(SIZE),
        hw.GetKnobValue(DENSE),
        hw.GetKnobValue(SPEED),
        hw.GetKnobValue(INDEX)
    );

    // Filter Drive zuweisen
    float drive_val = hw.GetKnobValue(REGEN) * 0.01f;
    dj_filter_l.SetDrive(drive_val);
    dj_filter_r.SetDrive(drive_val);

    dj_pot_val = hw.GetKnobValue(TONE); 

    // Filter Frequenz-Schnitt berechnen
    float filter_freq = max_freq;
    int filter_mode = 0; 

    if (dj_pot_val < 0.48f) {
        filter_mode = 1; 
        float norm = dj_pot_val / 0.48f;
        filter_freq = min_freq + (max_freq - min_freq) * (norm * norm);
    }
    else if (dj_pot_val > 0.52f) {
        filter_mode = 2; 
        float norm = (dj_pot_val - 0.52f) / 0.48f;
        filter_freq = min_freq + (max_freq - min_freq) * (norm * norm);
    }

    dj_filter_l.SetFreq(filter_freq);
    dj_filter_r.SetFreq(filter_freq);

    // Audio Block bearbeiten
    for (size_t i = 0; i < size; i++)
    {
        float sig_out_l = 0.0f;
        float sig_out_r = 0.0f;

        // 1. Reverb-Engine verarbeiten (True Stereo Input!)
        reverb.Process(in[0][i], in[1][i], out_gain, sig_out_l, sig_out_r);

        // 2. DJ-Filter auf das Wet-Signal anwenden
        if (filter_mode != 0) {
            dj_filter_l.Process(sig_out_l);
            sig_out_l = (filter_mode == 1) ? dj_filter_l.Low() : dj_filter_l.High();

            dj_filter_r.Process(sig_out_r);
            sig_out_r = (filter_mode == 1) ? dj_filter_r.Low() : dj_filter_r.High();
        }

        // 3. Finaler Stereo-Mix (Wet + Dry)
        out[0][i] = sig_out_l + ((1.0f - out_gain) * in[0][i]);
        out[1][i] = sig_out_r + ((1.0f - out_gain) * in[1][i]);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    
    // Reverb Klasse initialisieren
    reverb.Init(hw.AudioSampleRate());

    // DJ Filter initialisieren
    dj_filter_l.Init(hw.AudioSampleRate()); dj_filter_l.SetRes(0.05f);
    dj_filter_r.Init(hw.AudioSampleRate()); dj_filter_r.SetRes(0.05f);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while(1) {
        // Der Haupt-Loop bleibt frei für System-Tasks
        System::Delay(10);
    }
}