#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EditorComponents.h"

using Parameter = Graindr_PitchAudioProcessor::Parameter;

class Graindr_PitchAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Graindr_PitchAudioProcessorEditor (Graindr_PitchAudioProcessor&);
    ~Graindr_PitchAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    
    struct RoutingControls : public Component
    {
        explicit RoutingControls(const Graindr_PitchAudioProcessor::ParameterReferences& state)
            : dryWet(state.dryWet),
            ps1InBalance(state.ps1InBalance),
            ps2InBalance(state.ps2InBalance),
            psBalance(state.psBalance)
        {
            addAllAndMakeVisible(*this, dryWet, ps1InBalance, ps2InBalance, psBalance);
        }

        void resized() override
        {
            performLayout(getLocalBounds(), dryWet, ps1InBalance, ps2InBalance, psBalance);
        }

        AttachedSlider dryWet, ps1InBalance, ps2InBalance, psBalance;
    };
    
    struct PitchShifterControls : public Component
    {
        explicit PitchShifterControls(Parameter& _pitchShift, Parameter& _fineTune, Parameter& _grainSize, Parameter& _texture, Parameter& _strech, Parameter& _feedback, Parameter& _shimmer, Parameter& _shimmerHiCut, AudioParameterChoice& _playbackDir, AudioParameterChoice& _toneType)
            : pitchShift(_pitchShift),
            fineTune(_fineTune),
            grainSize(_grainSize),
            texture(_texture),
            strech(_strech),
            feedback(_feedback),
            shimmer(_shimmer),
            shimmerHiCut(_shimmerHiCut),
            playbackDir(_playbackDir),
            toneType(_toneType)
        {
            addAllAndMakeVisible(*this, pitchShift, fineTune, grainSize, texture, strech, feedback, shimmer, shimmerHiCut, playbackDir, toneType);
        }

        void resized() override
        {
            performLayout(getLocalBounds(), pitchShift, fineTune, grainSize, texture, strech, feedback, shimmer, shimmerHiCut, playbackDir, toneType);
        }

        AttachedSlider pitchShift, fineTune, grainSize, texture, strech, feedback, shimmer, shimmerHiCut;
        AttachedCombo playbackDir, toneType;
    };
    
    struct PitchShifterModControls : public Component
    {
        explicit PitchShifterModControls(const Graindr_PitchAudioProcessor::ParameterReferences& state)
            : modFreq(state.psModFreq),
            modDepth(state.psModDepth),
            modStereoPhase(state.psModStereoPhase),
            modWave(state.psModWave)
        {
            addAllAndMakeVisible(*this, modFreq, modDepth, modWave, modStereoPhase);
        }

        void resized() override
        {
            performLayout(getLocalBounds(), modFreq, modDepth, modWave, modStereoPhase);
        }

        AttachedSlider modFreq, modDepth, modStereoPhase;
        AttachedCombo modWave;
    };
    
    Graindr_PitchAudioProcessor& audioProcessor;
    
    RoutingControls routingControls { audioProcessor.getParameterValues() };
    
    PitchShifterControls ps1Controls { audioProcessor.getParameterValues().ps1PitchShift, audioProcessor.getParameterValues().ps1FineTune, audioProcessor.getParameterValues().ps1GrainSize, audioProcessor.getParameterValues().ps1Texture, audioProcessor.getParameterValues().ps1Strech, audioProcessor.getParameterValues().ps1Feedback, audioProcessor.getParameterValues().ps1Shimmer, audioProcessor.getParameterValues().ps1ShimmerHiCut, audioProcessor.getParameterValues().ps1PlaybackDir, audioProcessor.getParameterValues().ps1ToneType };
    
    PitchShifterControls ps2Controls { audioProcessor.getParameterValues().ps2PitchShift, audioProcessor.getParameterValues().ps2FineTune, audioProcessor.getParameterValues().ps2GrainSize, audioProcessor.getParameterValues().ps2Texture, audioProcessor.getParameterValues().ps2Strech, audioProcessor.getParameterValues().ps2Feedback, audioProcessor.getParameterValues().ps2Shimmer, audioProcessor.getParameterValues().ps2ShimmerHiCut, audioProcessor.getParameterValues().ps2PlaybackDir, audioProcessor.getParameterValues().ps2ToneType };
    
    PitchShifterModControls psModControls { audioProcessor.getParameterValues() };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Graindr_PitchAudioProcessorEditor)
};
