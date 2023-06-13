/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//create a data structure that holds value of our apvts 
struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts); //getter

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

    //cutFilter is used for low and high cut, with 12dB per filter

    //we can define an entire chain for mono signal to process it completely
    //Chain can define filters like lowpass, highpass, etc
    using monoChain = juce::dsp::ProcessorChain<cutFilter, Filter, cutFilter>;
    //monochain is lowCut->parametric->highCut
    //we need two mono to make stereo

    monoChain leftChain, rightChain;

    //define an enum so we know where we are in chain
    
    enum ChainPositions
    {
        lowCut,
        peak,
        highCut

    };

    //refactoring our code for filter

    void updatePeakFilter(const ChainSettings& chainSettings);

    using Coefficients = Filter::CoefficientsPtr; //making alias for JUCE reference

    //no member variables
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    

    //let's make another template to reduce code in switch below
    template<int Index,typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& cutCoeff) {
        updateCoefficients(chain.template get<Index>().coefficients, cutCoeff[Index]);
        chain.template setBypassed<Index>(false);
    }

    // because we don't know which type names are in JUCE,we can make template

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut,
        const CoefficientType& cutCoeff,
        const Slope& lowCutSlope)
    {
       // auto cutCoeff = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2 * (chainSettings.lowCutSlope + 1));

       // auto& leftLowCut = leftChain.get<ChainPositions::lowCut>();

        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);

        //switch (lowCutSlope) {
        //case Slope_12:
        //{
        //    //get the coefficient
        //    *leftLowCut.template get<0>().coefficients = *cutCoeff[0];
        //    //stop bypassing
        //    leftLowCut.template setBypassed<0>(false);
        //    break;
        //}
        //case Slope_24:
        //{
        //    *leftLowCut.template get<0>().coefficients = *cutCoeff[0];
        //    leftLowCut.template setBypassed<0>(false);
        //    *leftLowCut.template get<1>().coefficients = *cutCoeff[1];
        //    leftLowCut.template setBypassed<1>(false);

        //    break;
        //}
        //case Slope_36:
        //{
        //    *leftLowCut.template get<0>().coefficients = *cutCoeff[0];
        //    leftLowCut.template setBypassed<0>(false);
        //    *leftLowCut.template get<1>().coefficients = *cutCoeff[1];
        //    leftLowCut.template setBypassed<1>(false);
        //    *leftLowCut.template get<2>().coefficients = *cutCoeff[2];
        //    leftLowCut.template setBypassed<2>(false);
        //    break;
        //}
        //case Slope_48:
        //{
        //    *leftLowCut.template get<0>().coefficients = *cutCoeff[0];
        //    leftLowCut.template setBypassed<0>(false);
        //    *leftLowCut.template get<1>().coefficients = *cutCoeff[1];
        //    leftLowCut.template setBypassed<1>(false);
        //    *leftLowCut.template get<2>().coefficients = *cutCoeff[2];
        //    leftLowCut.template setBypassed<2>(false);
        //    *leftLowCut.template get<3>().coefficients = *cutCoeff[3];
        //    leftLowCut.template setBypassed<3>(false);
        //    break;
        //}

        //}

        //making switch easy to read by going top down
        //no break so we can carry the statement downward

        switch (lowCutSlope) {

        case Slope_48:
            {
            update<3>(leftLowCut, cutCoeff);
            
            }
            case Slope_36:
            {
            update<2>(leftLowCut, cutCoeff);

            }
            case Slope_24:
            {
            update<1>(leftLowCut, cutCoeff);
            }
            case Slope_12:
            {
            update<0>(leftLowCut, cutCoeff);
            }
        }
    }

    //let's refactor so we don't reuse code

    void updateLowCutFilter(const ChainSettings& chainSettings);
    void updateHighCutFilter(const ChainSettings& chainSettings);

    void updateFilter();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompASAudioProcessor)
};
