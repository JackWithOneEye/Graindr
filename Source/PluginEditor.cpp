#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Graindr_PitchAudioProcessorEditor::Graindr_PitchAudioProcessorEditor (Graindr_PitchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAllAndMakeVisible(*this, routingControls, ps1Controls, ps2Controls, psModControls);
    setSize(950, 700);
}

Graindr_PitchAudioProcessorEditor::~Graindr_PitchAudioProcessorEditor()
{
}

//==============================================================================
void Graindr_PitchAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    g.setColour(Colours::white);
    g.setFont(15.0f);
}

void Graindr_PitchAudioProcessorEditor::resized()
{
    const int rowHeight = 150;
    const auto rowWidth = getWidth() * 0.9f;
    auto y = 0;
    int rowhDy = 5;
    routingControls.setBounds(getWidth() * 0.05f, y, rowWidth * 0.5f, rowHeight);
    
    y = rowHeight + rowhDy;
    ps1Controls.setBounds(getWidth() * 0.05f, y, rowWidth, rowHeight);
    
    y += rowHeight + rowhDy;
    ps2Controls.setBounds(getWidth() * 0.05f, y, rowWidth, rowHeight);
    
    y += rowHeight + rowhDy;
    psModControls.setBounds(getWidth() * 0.05f, y, rowWidth * 0.67f, rowHeight);
}
