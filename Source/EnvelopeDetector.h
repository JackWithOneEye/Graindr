#pragma once

#include <JuceHeader.h>

enum DetectionMode
{
    MS,
    PEAK,
    RMS
};

struct EnvelopeDetectorOutput
{
    EnvelopeDetectorOutput() = default;
    EnvelopeDetectorOutput(float _envelope, bool _onAttack)
        : envelope(_envelope), onAttack(_onAttack) {}
    
    float envelope = 0.0f;
    bool onAttack = false;
};

class EnvelopeDetector
{
public:
    EnvelopeDetector() {}
    
    void reset(float sampleRate)
    {
        fs = sampleRate;
        lastEnvelope = 0.0f;
        lastState = State::REL;
        
        expFactor = TLD_AUDIO_ENVELOPE_ANALOG_TC * 1000.0f / fs;
    }
    
    void setParams(DetectionMode _dMode, float attackTime_ms, float releaseTime_ms, bool clamp)
    {
        dMode = _dMode;
        attackTime = std::expf(expFactor / attackTime_ms);
        releaseTime = std::expf(expFactor / releaseTime_ms);
        clampToOne = clamp;
    }
    
    std::vector<EnvelopeDetectorOutput> processBlock(float* buffer, int numSamples)
    {
        std::vector<EnvelopeDetectorOutput> envelope(numSamples);
        for (int n = 0; n < numSamples; n++)
        {
            envelope[n] = processSample(buffer[n]);
        }
        return envelope;
    }
    
    EnvelopeDetectorOutput processSample(float x);
    
private:
    enum State { ATK, REL };
    const float TLD_AUDIO_ENVELOPE_ANALOG_TC = std::log(0.368f);

    float fs = 44100.0f;
    float lastEnvelope = 0.0f;
    State lastState = State::REL;
    
    float expFactor = TLD_AUDIO_ENVELOPE_ANALOG_TC * 1000.0f / fs;
    
    DetectionMode dMode = DetectionMode::PEAK;
    float attackTime = 0.0f;
    float releaseTime = 0.0f;
    bool clampToOne = true;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeDetector)
};
