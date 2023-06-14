/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//define a datastructure for all our sliders once - : is used to initialize constructors and for inheritance
struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }

};

//==============================================================================
/**
*/
class CompASAudioProcessorEditor : public juce::AudioProcessorEditor,
    //inherit from listener class so processing can be done 
    //for editor's chain as well
    juce::AudioProcessorParameter::Listener,
    juce::Timer
    //we can't do GUI stuff in any callback by listener
    //we can update based on a flag which checks
{
public:
    CompASAudioProcessorEditor (CompASAudioProcessor&);
    ~CompASAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    //our timer will check whether parameter has changed and response curve needs updating
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CompASAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };

    //adding sliders for freq, gain, quality

    CustomRotarySlider peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider;


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

    monoChain MonoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompASAudioProcessorEditor)
};
