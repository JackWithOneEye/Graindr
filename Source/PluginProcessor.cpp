/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Graindr_PitchAudioProcessor::Graindr_PitchAudioProcessor(AudioProcessorValueTreeState::ParameterLayout layout)
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters { layout },
    vts(*this, nullptr, Identifier("Parameters"), std::move(layout))
{
        vts.state.addListener(this);
}

Graindr_PitchAudioProcessor::~Graindr_PitchAudioProcessor()
{
}

//==============================================================================
const String Graindr_PitchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Graindr_PitchAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Graindr_PitchAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Graindr_PitchAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

//==============================================================================
void Graindr_PitchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dwMixer.reset();
    dwMixer.prepare({ sampleRate, static_cast<uint32>(samplesPerBlock), 2 });
    dwMixer.setMixingRule(dsp::DryWetMixingRule::sin3dB);
    for (int i = 0; i < 2; i++)
    {
        psContainer[i].reset(sampleRate);
    }
}

void Graindr_PitchAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Graindr_PitchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Graindr_PitchAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (requiresUpdate.load())
    {
        dwMixer.setWetMixProportion(parameters.dryWet.get());
        for (int channel = 0; channel < 2; ++channel)
        {
            psContainer[channel].setRouting(
                parameters.ps1InBalance.get(),
                parameters.ps2InBalance.get(),
                parameters.psBalance.get()
            );
            psContainer[channel].setPs1Parameters(
                parameters.ps1GrainSize.get(),
                parameters.ps1PitchShift.get(),
                parameters.ps1FineTune.get(),
                parameters.ps1Texture.get(),
                parameters.ps1Strech.get(),
                parameters.ps1Feedback.get(),
                parameters.ps1Shimmer.get(),
                parameters.ps1ShimmerHiCut.get(),
                static_cast<PlaybackDirection>(parameters.ps1PlaybackDir.getIndex()),
                static_cast<ToneType>(parameters.ps1ToneType.getIndex())
            );
            psContainer[channel].setPs2Parameters(
                parameters.ps2GrainSize.get(),
                parameters.ps2PitchShift.get(),
                parameters.ps2FineTune.get(),
                parameters.ps2Texture.get(),
                parameters.ps2Strech.get(),
                parameters.ps2Feedback.get(),
                parameters.ps2Shimmer.get(),
                parameters.ps2ShimmerHiCut.get(),
                static_cast<PlaybackDirection>(parameters.ps2PlaybackDir.getIndex()),
                static_cast<ToneType>(parameters.ps2ToneType.getIndex())
            );
            psContainer[channel].setModParams(parameters.psModFreq.get(), parameters.psModDepth.get(), static_cast<FastMathLFO::LFOWave>(parameters.psModWave.getIndex()), channel == 0 ? 0.0f : parameters.psModStereoPhase.get()
            );
        }
        requiresUpdate.store(false);
    }
    
    dwMixer.pushDrySamples(buffer);
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        psContainer[channel].processBlock(channelData, buffer.getNumSamples());
    }
    dwMixer.mixWetSamples(buffer);
}

//==============================================================================
bool Graindr_PitchAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* Graindr_PitchAudioProcessor::createEditor()
{
    return new Graindr_PitchAudioProcessorEditor (*this);
}

//==============================================================================
void Graindr_PitchAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    copyXmlToBinary(*vts.copyState().createXml(), destData);
}

void Graindr_PitchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    vts.replaceState(ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes)));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Graindr_PitchAudioProcessor();
}
