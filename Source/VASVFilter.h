#pragma once

#include <JuceHeader.h>
#include "Utils.h"

class StaticVASVFilter
{
public:
    StaticVASVFilter() {}
    
    void reset(float sampleRate)
    {
        fs = sampleRate;
        sn_1 = 0.0f;
        sn_2 = 0.0f;
    }
    
    void setParameters(float _fc, float _q, bool _enableGainComp, bool _enableSoftClipper, float _bsfMix, float _bpfMix, float _hpfMix, float _lpfMix, bool _matchAnalogNyquistLPF)
    {
        fc = _fc;
        q = _q;
        enableGainComp = _enableGainComp;
        enableSoftClipper = _enableSoftClipper;
        
        bsfMix = _bsfMix;
        bpfMix = _bpfMix;
        hpfMix = _hpfMix;
        lpfMix = _lpfMix;
        
        matchAnalogNyquistLPF = _matchAnalogNyquistLPF;
        
        calcCoeffs();
    }
    
    float processSample(float x);
    
private:
    float fs = 44100.0f;
    
    float fc = 1000.0f;
    float q = 0.707f;
    bool enableGainComp = false;
    bool enableSoftClipper = false;
    
    float halfPeak = 1.0f;
    float alpha = 0.0f;
    float alpha_0 = 0.0f;
    float r = 0.707f;
    float rho = 1.414f;
    float sigma = 0.0f;
    
    float sn_1 = 0.0f;
    float sn_2 = 0.0f;
    
    float bsfMix = 0.0f;
    float bpfMix = 0.0f;
    float hpfMix = 0.0f;
    float lpfMix = 0.0f;
    
    bool matchAnalogNyquistLPF = true;
    
    void calcCoeffs();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticVASVFilter)
};

class VASVFilter
{
public:
    VASVFilter() {}
    
    void reset(float sampleRate)
    {
        fs = sampleRate;
        fc.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        q.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        
        bpfMix.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        bsfMix.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        hpfMix.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        lpfMix.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        
        sn_1 = 0.0f;
        sn_2 = 0.0f;
        
        calcCoeffs(true);    }
    
    void setParameters(float _fc, float _q, float _bpfMix, float _bsfMix, float _hpfMix, float _lpfMix)
    {
        fc.setTargetValue(_fc);
        q.setTargetValue(_q);
        
        bpfMix.setTargetValue(_bpfMix);
        bsfMix.setTargetValue(_bsfMix);
        hpfMix.setTargetValue(_hpfMix);
        lpfMix.setTargetValue(_lpfMix);
    }
    
    float processSample(float x);
    
private:
    float fs = 44100.0f;
    
    float currentFc = 0.0f;
    SmoothedValM fc = 1000.0f;
    
    float currentQ = 0.0f;
    SmoothedVal q = 0.707f;
    
    float halfPeak = 1.0f;
    float alpha = 0.0f;
    float alpha_0 = 0.0f;
    float r = 0.707f;
    float rho = 1.414f;
    float sigma = 0.0f;
    
    SmoothedVal bsfMix = 0.0f;
    SmoothedVal bpfMix = 0.0f;
    SmoothedVal hpfMix = 0.0f;
    SmoothedVal lpfMix = 0.0f;
    
    float sn_1 = 0.0f;
    float sn_2 = 0.0f;
    
    void calcCoeffs(bool force = false);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VASVFilter)
};
