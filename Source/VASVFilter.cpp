#include "VASVFilter.h"

inline static float peakGainForQ(float q)
{
    if (q <= 0.707)
        return 1.0f;
    
    auto q2 = q * q;
    return q2 / std::pow(q2 - 0.25f, 0.5f);
}

void StaticVASVFilter::calcCoeffs()
{
    alpha = dsp::FastMathApproximations::tan((MathConstants<float>::pi * fc) / fs);
    r = 1.0f / (2.0f * q);
    rho = 2.0f * r + alpha;
    alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
    sigma = (4.0f * fc * fc) / (alpha * fs * fs);
    
    halfPeak = 1.0f;
    auto peak_dB = Decibels::gainToDecibels(peakGainForQ(q));
    if (peak_dB > 0.0f)
        halfPeak = Decibels::decibelsToGain(-peak_dB * 0.5f);
}

float StaticVASVFilter::processSample(float x)
{
    if (enableGainComp)
        x *= halfPeak;
    
    auto hpf = alpha_0 * (x - rho * sn_1 - sn_2);
    auto bpf = alpha * hpf + sn_1;
    if (enableSoftClipper)
        bpf = std::tanh(bpf);
    
    auto lpf = alpha * bpf + sn_2;
    auto bsf = hpf + lpf;
    auto lpf2 = matchAnalogNyquistLPF ? lpf + sigma * sn_1 : lpf;
        
    sn_1 = alpha * hpf + bpf;
    sn_2 = alpha * bpf + lpf;
        
    return bsfMix * bsf + bpfMix * bpf + hpfMix * hpf + lpfMix * lpf2;
}

void VASVFilter::calcCoeffs(bool force)
{
    if (!force && !fc.isSmoothing() && !q.isSmoothing())
        return;
    
    if (force || fc.isSmoothing())
    {
        auto currentFc = fc.getNextValue();
        // alpha = std::tan((MathConstants<float>::pi * currentFc) / fs);
        alpha = dsp::FastMathApproximations::tan((MathConstants<float>::pi * currentFc) / fs);
        sigma = (4.0f * currentFc * currentFc) / (alpha * fs * fs);
    }
    
    if (force || q.isSmoothing())
    {
        auto currentQ = q.getNextValue();
        auto peak_dB = Decibels::gainToDecibels(peakGainForQ(currentQ));
        if (peak_dB > 0.0f)
            halfPeak = Decibels::decibelsToGain(-peak_dB * 0.5f);
        
        r = 1.0f / (2.0f * currentQ);
    }
    
    rho = 2.0f * r + alpha;
    alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
}

float VASVFilter::processSample(float x)
{
    calcCoeffs();
    
    // x *= halfPeak;
    auto hpf = alpha_0 * (x - rho * sn_1 - sn_2);
    auto bpf = alpha * hpf + sn_1; // std::tanh(alpha * hpf + sn_1[n]);
    auto lpf = alpha * bpf + sn_2;
    auto bsf = hpf + lpf;
    auto lpf2 = lpf + sigma * sn_1;
    
    sn_1 = alpha * hpf + bpf;
    sn_2 = alpha * bpf + lpf;
    
    return bpfMix.getNextValue() * bpf + bsfMix.getNextValue() * bsf + hpfMix.getNextValue() * hpf + lpfMix.getNextValue() * lpf2;
}
