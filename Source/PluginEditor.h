/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//define a datastructure for all our sliders once - : is used to initialize constructors and for inheritance

//make a look and feel class to inherit functions

struct LookAndFeel : juce::LookAndFeel_V4 {
    void drawRotarySlider(juce::Graphics&, int x, int y,
        int width,
        int height, 
        float sliderPosProportional,
        float rotaryStartAngle, 
        float rotaryEndAngle, 
        juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels() {
        setLookAndFeel(NULL);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
   
private:
    //base class for methods like getValue, setValue, convert0to1 etc
    juce::RangedAudioParameter* param;
    juce::String suffix;
    LookAndFeel lnf;
};

struct ResponseCurveComponent : juce::Component, //inherit from listener class so processing can be done 
    //for editor's chain as well
    juce::AudioProcessorParameter::Listener,
    juce::Timer
    //we can't do GUI stuff in any callback by listener
    //we can update based on a flag which checks
{
    ResponseCurveComponent(CompASAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    //our timer will check whether parameter has changed and response curve needs updating
    void timerCallback() override;

    void paint(juce::Graphics& g) override;

   

private:
    CompASAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };
    monoChain MonoChain;
    void updateChain();
};

//==============================================================================
/**
*/
class CompASAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CompASAudioProcessorEditor (CompASAudioProcessor&);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CompASAudioProcessor& audioProcessor;

    //adding sliders for freq, gain, quality

    RotarySliderWithLabels peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    //to connect sliders to control, we can use apvts

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    //one attachment for each slider
    Attachment  peakFreqSliderAttachment,
            peakGainSliderAttachment,
            peakQualitySliderAttachment,
            lowCutFreqSliderAttachment,
            highCutFreqSliderAttachment,
            lowCutSlopeSliderAttachment,
            highCutSlopeSliderAttachment;

    //all components have same thing to be done to them, so
    //we can make vector and have it iterate over 
    std::vector<juce::Component*> getComps();

   // monoChain MonoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompASAudioProcessorEditor)
};
