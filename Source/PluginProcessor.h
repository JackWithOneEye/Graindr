#pragma once

#include <JuceHeader.h>
#include "GranularPitchShifter.h"
#include "PitchShifterContainer.h"

namespace paramID
{
#define PARAMETER_ID(str) constexpr const char* str { #str };
    PARAMETER_ID(dryWet)
    PARAMETER_ID(ps1InBalance)
    PARAMETER_ID(ps2InBalance)
    PARAMETER_ID(psBalance)

    // PS 1
    PARAMETER_ID(ps1PitchShift)
    PARAMETER_ID(ps1FineTune)
    PARAMETER_ID(ps1GrainSize)
    PARAMETER_ID(ps1Texture)
    PARAMETER_ID(ps1Strech)
    PARAMETER_ID(ps1Feedback)
    PARAMETER_ID(ps1Shimmer)
    PARAMETER_ID(ps1ShimmerHiCut)
    PARAMETER_ID(ps1PlaybackDir)
    PARAMETER_ID(ps1ToneType)

    // PS 2
    PARAMETER_ID(ps2PitchShift)
    PARAMETER_ID(ps2FineTune)
    PARAMETER_ID(ps2GrainSize)
    PARAMETER_ID(ps2Texture)
    PARAMETER_ID(ps2Strech)
    PARAMETER_ID(ps2Feedback)
    PARAMETER_ID(ps2Shimmer)
    PARAMETER_ID(ps2ShimmerHiCut)
    PARAMETER_ID(ps2PlaybackDir)
    PARAMETER_ID(ps2ToneType)

    PARAMETER_ID(psModFreq)
    PARAMETER_ID(psModDepth)
    PARAMETER_ID(psModWave)
    PARAMETER_ID(psModStereoPhase)
#undef PARAMETER_ID
}

template <typename Func, typename... Items>
constexpr void forEach(Func&& func, Items&&... items)
noexcept (noexcept (std::initializer_list<int> { (func(std::forward<Items>(items)), 0)... }))
{
    (void)std::initializer_list<int> { ((void)func(std::forward<Items>(items)), 0)... };
}

class Graindr_PitchAudioProcessor : public AudioProcessor, private ValueTree::Listener
{
public:
    //==============================================================================
    explicit Graindr_PitchAudioProcessor(AudioProcessorValueTreeState::ParameterLayout layout);
    Graindr_PitchAudioProcessor() : Graindr_PitchAudioProcessor(AudioProcessorValueTreeState::ParameterLayout{}) {};
    ~Graindr_PitchAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; };

    //==============================================================================
    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram(int index) override {};
    const String getProgramName(int index) override { return {}; };
    void changeProgramName(int index, const String& newName) override {};

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    using Parameter = AudioProcessorValueTreeState::Parameter;
    struct ParameterReferences
    {
        template <typename Param>
        static Param& addToLayout(AudioProcessorValueTreeState::ParameterLayout& layout, std::unique_ptr<Param> param)
        {
            auto& ref = *param;
            layout.add(std::move(param));
            return ref;
        }
        static String valueToTextFunction(float x) { return String(x, 2); }
        static float textToValueFunction(const String& str) { return str.getFloatValue(); }
        
        static const StringArray playbackDirOptions() { return StringArray{ "FORWARD", "REVERSE", "ALTERNATE" }; }
        static const StringArray toneTypeOptions() { return StringArray{ "ANALOG", "DIGITAL" }; }
        static const StringArray lfoWaveformOptions() { return StringArray{ "SIN", "TRI", "SAW", "RMP", "SQR", "Random S&H" }; }

        explicit ParameterReferences(AudioProcessorValueTreeState::ParameterLayout& layout)
            : dryWet(addToLayout(layout, std::make_unique<Parameter>(paramID::dryWet, "Dry/Wet", "", NormalisableRange<float>(0.0f, 1.0f), 0.5f, valueToTextFunction, textToValueFunction))),
            ps1InBalance(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1InBalance, "PS 1 In", "", NormalisableRange<float>(0.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps2InBalance(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2InBalance, "PS 2 In", "", NormalisableRange<float>(0.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            psBalance(addToLayout(layout, std::make_unique<Parameter>(paramID::psBalance, "PS Balance", "", NormalisableRange<float>(0.0f, 1.0f), 0.5f, valueToTextFunction, textToValueFunction))),
        
            ps1PitchShift(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1PitchShift, "Pitch Shift", "", NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps1FineTune(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1FineTune, "Fine Tune", "", NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps1GrainSize(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1GrainSize, "Grain Size", "ms", NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 50.0f, valueToTextFunction, textToValueFunction))),
            ps1Texture(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1Texture, "Texture", "", NormalisableRange<float>(0.0f, 1.0f), 0.5f, valueToTextFunction, textToValueFunction))),
            ps1Strech(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1Strech, "Strech", "", NormalisableRange<float>(1.0f, 4.0f, 1.0f), 1.0f, valueToTextFunction, textToValueFunction))),
            ps1Feedback(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1Feedback, "Feedback", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps1Shimmer(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1Shimmer, "Shimmer", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps1ShimmerHiCut(addToLayout(layout, std::make_unique<Parameter>(paramID::ps1ShimmerHiCut, "Hi Cut", "Hz", NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f), 22000.0f, valueToTextFunction, textToValueFunction))),
            ps1PlaybackDir(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::ps1PlaybackDir, "Playback", playbackDirOptions(), 0))),
            ps1ToneType(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::ps1ToneType, "Tone", toneTypeOptions(), 1))),
        
            ps2PitchShift(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2PitchShift, "Pitch Shift", "", NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps2FineTune(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2FineTune, "Fine Tune", "", NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps2GrainSize(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2GrainSize, "Grain Size", "ms", NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 50.0f, valueToTextFunction, textToValueFunction))),
            ps2Texture(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2Texture, "Texture", "", NormalisableRange<float>(0.0f, 1.0f), 0.5f, valueToTextFunction, textToValueFunction))),
            ps2Strech(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2Strech, "Strech", "", NormalisableRange<float>(1.0f, 4.0f, 1.0f), 1.0f, valueToTextFunction, textToValueFunction))),
            ps2Feedback(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2Feedback, "Feedback", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps2Shimmer(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2Shimmer, "Shimmer", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            ps2ShimmerHiCut(addToLayout(layout, std::make_unique<Parameter>(paramID::ps2ShimmerHiCut, "Hi Cut", "Hz", NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f), 22000.0f, valueToTextFunction, textToValueFunction))),
            ps2PlaybackDir(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::ps2PlaybackDir, "Playback", playbackDirOptions(), 0))),
            ps2ToneType(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::ps2ToneType, "Tone", toneTypeOptions(), 1))),
        
            psModFreq(addToLayout(layout, std::make_unique<Parameter>(paramID::psModFreq, "Mod Freq", "Hz", NormalisableRange<float>(MIN_LFO_FREQ, MAX_LFO_FREQ), MIN_LFO_FREQ, valueToTextFunction, textToValueFunction))),
            psModDepth(addToLayout(layout, std::make_unique<Parameter>(paramID::psModDepth, "Mod Depth", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
            psModWave(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::psModWave, "Mod Wave", lfoWaveformOptions(), 0))),
            psModStereoPhase(addToLayout(layout, std::make_unique<Parameter>(paramID::psModStereoPhase, "Mod Stereo Phase Shift", "Deg", NormalisableRange<float>(0.0f, 180.0f, 1.0f), 0.0f, valueToTextFunction, textToValueFunction)))
        {}

        Parameter& dryWet;
        Parameter& ps1InBalance;
        Parameter& ps2InBalance;
        Parameter& psBalance;
        
        Parameter& ps1PitchShift;
        Parameter& ps1FineTune;
        Parameter& ps1GrainSize;
        Parameter& ps1Texture;
        Parameter& ps1Strech;
        Parameter& ps1Feedback;
        Parameter& ps1Shimmer;
        Parameter& ps1ShimmerHiCut;
        AudioParameterChoice& ps1PlaybackDir;
        AudioParameterChoice& ps1ToneType;
        
        Parameter& ps2PitchShift;
        Parameter& ps2FineTune;
        Parameter& ps2GrainSize;
        Parameter& ps2Texture;
        Parameter& ps2Strech;
        Parameter& ps2Feedback;
        Parameter& ps2Shimmer;
        Parameter& ps2ShimmerHiCut;
        AudioParameterChoice& ps2PlaybackDir;
        AudioParameterChoice& ps2ToneType;
        
        Parameter& psModFreq;
        Parameter& psModDepth;
        AudioParameterChoice& psModWave;
        Parameter& psModStereoPhase;
    };
    
    const ParameterReferences& getParameterValues() const noexcept { return parameters; }
    AudioProcessorValueTreeState& getVts() { return vts; }

private:
    void valueTreePropertyChanged(ValueTree&, const Identifier&) override
    {
        requiresUpdate.store(true);
    }

    ParameterReferences parameters;
    AudioProcessorValueTreeState vts;
    std::atomic<bool> requiresUpdate{ true };
    
    dsp::DryWetMixer<float> dwMixer;
    PitchShifterContainer psContainer[2];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Graindr_PitchAudioProcessor)
};
