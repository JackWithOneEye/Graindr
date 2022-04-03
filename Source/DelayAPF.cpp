#include "DelayAPF.h"

DelayAPF::DelayAPF() {}

float DelayAPF::applyLpf(float x)
{
    lpf_state = (x * (1.0f - lpf_gain)) + (lpf_state * lpf_gain);
    return lpf_state;
}

float DelayAPF::process(float x)
{
    auto dl_out = delayLine.readBuffer(delaySamples);
    x += (apf_gain * dl_out);
    auto yn = (x * -apf_gain) + dl_out;

    auto dl_in = applyLpf(x);
    delayLine.writeBuffer(dl_in);

    return yn;
}

void DelayAPF::reset(float sampleRate, float _delaySamples)
{
    fs = sampleRate;
    lpf_state = 0.0;
    delaySamples = _delaySamples;
    delayLine.createCircularBuffer(static_cast<int>(delaySamples + 0.5f));
}

NestedDelayAPF::NestedDelayAPF() :
    DelayAPF()
{}

float NestedDelayAPF::process(float x)
{
    auto dl_out = delayLine.readBuffer(delaySamples);
    x += (apf_gain * dl_out);
    auto yn = (x * -apf_gain) + dl_out;

    auto nested_in = applyLpf(x);

    auto nested_dl_out = nestedDelayLine.readBuffer(nested_delaySamples);
    nested_in += (nested_apf_gain * nested_dl_out);
    delayLine.writeBuffer(nested_in);
    auto dl_in = (nested_in * -nested_apf_gain) + nested_dl_out;
    delayLine.writeBuffer(dl_in);

    return yn;
}

void NestedDelayAPF::reset(float sampleRate, float _delaySamples, float _nested_delaySamples)
{
    DelayAPF::reset(sampleRate, _delaySamples);
    nested_delaySamples = _nested_delaySamples;
    nestedDelayLine.createCircularBuffer(static_cast<int>(nested_delaySamples + 0.5f));
}
