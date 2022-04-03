#pragma once

#include <JuceHeader.h>

using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

template <typename... Components>
void addAllAndMakeVisible(Component& target, Components&... children)
{
    forEach([&](Component& child) { target.addAndMakeVisible(child); }, children...);
}

class AttachedSlider : public Component
{
public:
    explicit AttachedSlider(RangedAudioParameter& param)
        : label("", param.name),
        attachment(param, slider)
    {
        addAllAndMakeVisible(*this, slider, label);

        slider.setTextValueSuffix(" " + param.label);

        label.attachToComponent(&slider, false);
        label.setJustificationType(Justification::centred);
    }

    void resized() override { slider.setBounds(getLocalBounds().reduced(0, 40)); }

    Slider& getSlider() { return slider; }

private:
    Slider slider{ Slider::RotaryVerticalDrag, Slider::TextBoxBelow };
    Label label;
    SliderParameterAttachment attachment;
};

class AttachedCombo : public Component
{
public:
    explicit AttachedCombo(RangedAudioParameter& param)
        : combo(param),
        label("", param.name),
        attachment(param, combo)
    {
        addAllAndMakeVisible(*this, combo, label);

        label.attachToComponent(&combo, false);
        label.setJustificationType(Justification::centred);
    }

    void resized() override
    {
        combo.setBounds(getLocalBounds().withSizeKeepingCentre(jmin(getWidth(), 150), 24));
    }

    ComboBox& getComboBox() { return combo; }

private:
    struct ComboWithItems : public ComboBox
    {
        explicit ComboWithItems(RangedAudioParameter& param)
        {
            addItemList(dynamic_cast<AudioParameterChoice&> (param).choices, 1);
        }
    };

    ComboWithItems combo;
    Label label;
    ComboBoxParameterAttachment attachment;
};

class AttachedToggle : public Component
{
public:
    AttachedToggle(RangedAudioParameter& param)
        : toggle(param.name),
          attachment(param, toggle)
    {
        toggle.addMouseListener (this, true);
        addAndMakeVisible(toggle);
    }

    void resized() override { toggle.setBounds (getLocalBounds()); }

private:
    ToggleButton toggle;
    ButtonParameterAttachment attachment;
};

struct GetTrackInfo
{
    // Combo boxes need a lot of room
    Grid::TrackInfo operator() (AttachedCombo&)             const { return 120_px; }
    
    // Toggles are a bit smaller
    Grid::TrackInfo operator() (AttachedToggle&)            const { return 80_px; }
    
    // Sliders take up as much room as they can
    Grid::TrackInfo operator() (AttachedSlider&)            const { return 1_fr; }
};

template <typename... Components>
static void performLayout(const Rectangle<int>& bounds, Components&... components)
{
    Grid grid;
    using Track = Grid::TrackInfo;

    grid.autoColumns = Track(1_fr);
    grid.autoRows = Track(1_fr);
    grid.columnGap = Grid::Px(10);
    grid.rowGap = Grid::Px(0);
    grid.autoFlow = Grid::AutoFlow::column;

    grid.templateColumns = { GetTrackInfo{} (components)... };
    grid.items = { GridItem(components)... };

    grid.performLayout(bounds);
}
