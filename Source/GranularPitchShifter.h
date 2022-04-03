#pragma once

#include <JuceHeader.h>
#include "Utils.h"
#include "VASVFilter.h"
#include "DelayAPF.h"

constexpr int MAX_GRAIN_SIZE_SEC = 1;

constexpr int MIN_PITCH_SHIFT = -12;
constexpr int MAX_PITCH_SHIFT = 12;

constexpr float MIN_PITCH_SHIFT_FACTOR = 0.5f;
constexpr float MAX_PITCH_SHIFT_FACTOR = 2.0f;

constexpr float MIN_LFO_FREQ = 0.02f;
constexpr float MAX_LFO_FREQ = 20.0f;

inline static float pitchShift2Factor(float shift) { return std::powf(2.0f, shift / 12.0f); }

inline static void resizeAndInitBuffer(std::vector<float>& v, int size)
{
    v.resize(size);
    std::fill(v.begin(), v.end(), 0.0f);
}

enum PlaybackDirection
{
    FORWARD,
    REVERSE,
    ALTERNATE
};

enum ToneType
{
    ANALOG,
    DIGITAL
};

class GranularPitchShifter
{
public:
    GranularPitchShifter() {}
    
    void reset(float sampleRate)
    {
        fs = sampleRate;
        
        int maxOutputBufferSize = ((int) fs) * MAX_GRAIN_SIZE_SEC;
        int maxInputBufferSize = ((int) MAX_PITCH_SHIFT_FACTOR) * maxOutputBufferSize;
        inputBuffer.createCircularBuffer(maxInputBufferSize * 2);
        outputBuffer1.createCircularBuffer(maxOutputBufferSize * 4);
        outputBuffer2.createCircularBuffer(maxOutputBufferSize * 4);
        postDelayBuffer.createCircularBuffer(maxOutputBufferSize * 2);
        
        writtenSmplsCtr = 0;
        stretchCtr = 0;
        
        output1ReadIdx = 0;
        output1ReadOffset = 0;
        output1StretchMultiplier = 1;
        output1TriggerStretchOnNext = false;
        
        output2ReadIdx = 0;
        output2ReadOffset = 0;
        output2StretchMultiplier = 1;
        output2TriggerStretchOnNext = false;
//        currentStride = -1;
        
        grainSize_smpls.reset(fs, 1.0f);
        pitchShiftFactor.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        fineTuneFactor.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        texture.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        nextStretch = stretch;
        feedback.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        shimmer.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        
        psBandpass.reset(fs);
        psBandpass.setParameters(725.0f, 0.33f, false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);
        
        postDelayBandpass.reset(fs);
        postDelayBandpass.setParameters(725.0f, 0.33f, false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);
        
        fdbkHpf.reset(fs);
        fdbkHpf.setParameters(100.0f, 0.707f, false, false, 0.0f, 0.0f, 1.0f, 0.0f, false);
        
        postDelayHpf.reset(fs);
        postDelayHpf.setParameters(100.0f, 0.707f, false, false, 0.0f, 0.0f, 1.0f, 0.0f, false);
        
        fdbkLpf.reset(fs);
        
//        nDelayApf.reset(fs, 149.0f, 239.0f);
//        nDelayApf.setParams(0.9f, 0.5f);
    }
    
    void setParameters(float grainSize_ms, float _pitchShift, float _fineTune, float _texture, float _strech, float _feedback, float _shimmer, float shimmerHiCut_Hz, PlaybackDirection _playbackDir, ToneType _toneType)
    {
        grainSize_smpls.setTargetValue(grainSize_ms * 0.001f * fs);
        
        if (_pitchShift != pitchShift)
        {
            pitchShift = _pitchShift;
            pitchShiftFactor.setTargetValue(pitchShift2Factor(pitchShift));
        }
        if (_fineTune != fineTune)
        {
            fineTune = _fineTune;
            fineTuneFactor.setTargetValue(pitchShift2Factor(fineTune * 0.01f));
        }
        
        texture.setTargetValue(_texture);
        nextStretch = (int) _strech;
        feedback.setTargetValue(jlimit(0.0f, 1.0f, _feedback * 0.01f));
        shimmer.setTargetValue(jlimit(0.0f, 1.0f, _shimmer * 0.01f));
        fdbkLpf.setParameters(shimmerHiCut_Hz, 0.707f, 0.0f, 0.0f, 0.0f, 1.0f);
        playbackDir = _playbackDir;
        toneType = _toneType;
    }
    
    void processBlock(float* buffer, const int numSamples);
    
    float processSample(float x, float modulation = 0.0f, bool triggerStrech = false);
    
    
    float processOutputBuffers(float modulation, bool triggerStrech);
    float processPostDelay(float x);
    void writeInputBuffer(float x, float feedbackIn);
    
private:
    static constexpr float ANALOG_POST_DEL_LOOP_GAIN = 3.98f;
    float fs = 44100.0f;
    
    SmoothedValM grainSize_smpls = 2205.0f;
    
    float pitchShift = 0;
    SmoothedValM pitchShiftFactor = 1.0f;
    float fineTune = 0;
    SmoothedValM fineTuneFactor = 1.0f;
    SmoothedVal texture = 0.5f;
    
    int stretch = 1;
    int nextStretch = 1;
    
    SmoothedVal feedback = 0.0f;
    SmoothedVal shimmer = 0.0f;
    PlaybackDirection playbackDir = PlaybackDirection::FORWARD;
    ToneType toneType = ToneType::DIGITAL;
    
    int writtenSmplsCtr = 0;
    int stretchCtr = 0;
    
    int output1ReadIdx = 0;
    int output1ReadOffset = 0;
    int output1StretchMultiplier = 1;
    bool output1TriggerStretchOnNext = false;
    
    int output2ReadIdx = 0;
    int output2ReadOffset = 0;
    int output2StretchMultiplier = 1;
    bool output2TriggerStretchOnNext = false;
    
    float currentInputSize = 0.0f;
    int currentOutputSize = 0;
    
    CircularBuffer inputBuffer;
    CircularBuffer outputBuffer1;
    CircularBuffer outputBuffer2;
    CircularBuffer postDelayBuffer;
    
    StaticVASVFilter psBandpass;
    StaticVASVFilter postDelayBandpass;
    StaticVASVFilter fdbkHpf;
    StaticVASVFilter postDelayHpf;
    
    VASVFilter fdbkLpf;
    
//    NestedDelayAPF nDelayApf;
    
    inline float softClipper(float x)
    {
        return std::tanh(x);
    }
    
    inline void resample(float inputSize, int outputSize, float factor);
    inline float windowFunc(int n, int outputSize, int overlapWidth);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularPitchShifter)
};
