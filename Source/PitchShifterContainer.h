#pragma once

#include <JuceHeader.h>
#include "GranularPitchShifter.h"
#include "EnvelopeDetector.h"

class PitchShifterContainer
{
public:
    PitchShifterContainer() {}
    
    void reset(float sampleRate)
    {
        ps1.reset(sampleRate);
        ps2.reset(sampleRate);
        
        ps1InBalance.reset(sampleRate, 0.1f);
        ps2InBalance.reset(sampleRate, 0.1f);
        ps12OutBalance.reset(sampleRate, 0.1f);
        
        lfoPhaseShift.reset(sampleRate, 0.1f);
        lfo.reset(sampleRate);
        
        preDetectorFilter.reset(sampleRate);
        preDetectorFilter.setParameters(1500.0f, 0.707f, false, false, 0.0f, 0.0f, 0.0f, 1.0f, false);
        envelopeDetector.reset(sampleRate);
        envelopeDetector.setParams(DetectionMode::RMS, 1.5f, 5.0f, false);
        noteOn = false;
    }
    
    void processBlock(float* buffer, const int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            auto x = buffer[n];
            
            auto envlpIn = preDetectorFilter.processSample(x);
            auto envlpOut = envelopeDetector.processSample(envlpIn);
            auto delta = envlpOut.envelope - TRACKING_THRESHOLD;
            float modVal = 0.0f;
            if (delta > 0)
                modVal = jlimit(0.0f, 1.0f, delta * TRACKING_SENSITIVITY);
            
            bool triggerStrech = false;
            if (!noteOn && modVal >= 0.001f)
            {
                noteOn = true;
                triggerStrech = true;
            }
            else if (noteOn && modVal < 0.001f)
            {
                noteOn = false;
            }
            
            // auto mod = modVal * (maxValue - minValue) + minValue;
            
            auto lfoOut = lfo.getNextSample(lfoPhaseShift.getNextValue());
            
            // PS 1 process
            auto ps1OutputBuffersOut = ps1.processOutputBuffers(lfoOut, triggerStrech);
            auto ps1PostDelOut = ps1.processPostDelay(ps1OutputBuffersOut);
            
            // PS 2 process
            auto ps2OutputBuffersOut = ps2.processOutputBuffers(lfoOut, triggerStrech);
            auto ps2PostDelOut = ps2.processPostDelay(ps2OutputBuffersOut);

            // PS 1 write
            auto ps1InBal = ps1InBalance.getNextValue();
            ps1.writeInputBuffer((1.0f - ps1InBal) * x + ps1InBal * ps2OutputBuffersOut, ps1OutputBuffersOut);
            
            // PS 2 write
            auto ps2InBal = ps2InBalance.getNextValue();
            ps2.writeInputBuffer((1.0f - ps2InBal) * x + ps2InBal * ps1OutputBuffersOut, ps2OutputBuffersOut);
            
            auto outBal = ps12OutBalance.getNextValue();
            buffer[n] = (1.0f - outBal) * ps1PostDelOut + outBal * ps2PostDelOut;
        }
    }
    
    void setRouting(float _ps1InBalance, float _ps2InBalance, float _ps12OutBalance)
    {
        ps1InBalance.setTargetValue(_ps1InBalance);
        ps2InBalance.setTargetValue(_ps2InBalance);
        ps12OutBalance.setTargetValue(_ps12OutBalance);
    }
    
    void setPs1Parameters(float grainSize_ms, float pitchShift, float fineTune, float texture, float strech, float feedback, float shimmer, float shimmerHiCut_Hz, PlaybackDirection playbackDir, ToneType toneType)
    {
        ps1.setParameters(grainSize_ms, pitchShift, fineTune, texture, strech, feedback, shimmer, shimmerHiCut_Hz, playbackDir, toneType);
    }
    
    void setPs2Parameters(float grainSize_ms, float pitchShift, float fineTune, float texture, float strech, float feedback, float shimmer, float shimmerHiCut_Hz, PlaybackDirection playbackDir, ToneType toneType)
    {
        ps2.setParameters(grainSize_ms, pitchShift, fineTune, texture, strech, feedback, shimmer, shimmerHiCut_Hz, playbackDir, toneType);
    }
    
    void setModParams(float freq, float depth, FastMathLFO::LFOWave wave, float phaseShift)
    {
        lfo.setParams(freq, depth, wave, FastMathLFO::LFOPolarity::UNIPOLAR);
        lfoPhaseShift.setTargetValue(degreesToRadians(phaseShift));
    }
    
private:
    static constexpr float TRACKING_THRESHOLD = 0.001f;
    static constexpr float TRACKING_SENSITIVITY = 0.25f;
    
    GranularPitchShifter ps1;
    GranularPitchShifter ps2;
    
    SmoothedVal ps1InBalance = 0.0f;
    SmoothedVal ps2InBalance = 0.0f;
    SmoothedVal ps12OutBalance = 0.5f;
    
    FastMathLFO lfo;
    SmoothedVal lfoPhaseShift = 0.0f;
    
    StaticVASVFilter preDetectorFilter;
    
    EnvelopeDetector envelopeDetector;
    bool noteOn = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShifterContainer)
};
