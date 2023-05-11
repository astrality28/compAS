/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class CompASAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    CompASAudioProcessor();
    ~CompASAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); //static because it uses no data members
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()}; //apvts basically controls all the audio parameters of the project
    //we pass this as the plugin, parameters are passed by the createParameterLayout function

private:
    //creating alias to avoid using the entire namespace
    using Filter = juce::dsp::IIR::Filter<float>; //auto response of 12db/Oct, so if we need like 48, then we can use it 4 times
    //we can pass context of one, and then it chains 4 times so we get 48dB/Oct
    //context here is Filter alias
    using cutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    //we can define an entire chain for mono signal to process it completely
    //Chain can define filters like lowpass, highpass, etc
    using monoChain = juce::dsp::ProcessorChain<cutFilter, Filter, cutFilter>;
    //monochain is lowCut->parametric->highCut
    //we need two mono to make stereo

    monoChain leftChain, rightChain;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompASAudioProcessor)
};
