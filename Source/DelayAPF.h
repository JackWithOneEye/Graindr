#pragma once

#include "JuceHeader.h"
#include "Utils.h"

class DelayAPF
{
public:
    DelayAPF();

    float process(float x);

    void reset(float sampleRate, float _delaySamples);

    void setParams(float gain, float damping)
    {
        apf_gain = jlimit(0.5f, 0.707f, gain);
        lpf_gain = jlimit(0.0f, 0.99999f, damping);
    }

protected:
    float fs = 44100.0f;
    float delaySamples = 0.0f;
    float apf_gain = 0.0f;

    float lpf_state = 0.0f;
    float lpf_gain = 0.0f;

    CircularBuffer delayLine;

    float applyLpf(float x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayAPF);
};

class NestedDelayAPF : public DelayAPF
{
public:
    NestedDelayAPF();

    float process(float x);

    void reset(float sampleRate, float _delaySamples, float _nested_delaySamples);

    void setParams(float gain, float damping)
    {
        DelayAPF::setParams(gain, damping);
        nested_apf_gain = jlimit(0.5f, 0.707f, gain);
    }

private:
    float nested_delaySamples = 0.0f;
    float nested_apf_gain = 0.0f;

    CircularBuffer nestedDelayLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NestedDelayAPF);
};
