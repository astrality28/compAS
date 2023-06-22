/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//class below retrieves the blocks of buffer from the below fifo

#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

//two FFTs, one for each channel
enum Channel {
    Right,
    Left
};


//Since FFTs work in particular buffer sizes, this class does that work for us

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,             //channel
            bufferSize,    //num samples
            false,         //keepExistingContent
            true,          //clear extra space
            true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

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

//make monochain public for response curve

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

enum ChainPositions
{
    lowCut,
    peak,
    highCut

};

using Coefficients = Filter::CoefficientsPtr; //making alias for JUCE reference

//no member variables
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

template<int Index, typename ChainType, typename CoefficientType>
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

//use inline so linker knows where implementation is done
inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));
}

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


    //we make public instances of fifo cause gui needs access too

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right };

    

private:

    monoChain leftChain, rightChain;
    //refactoring our code for filter

    //static void updateCoefficients(Coefficients& old, const Coefficients& replacements);


    void updatePeakFilter(const ChainSettings& chainSettings);

    //let's refactor so we don't reuse code

    void updateLowCutFilter(const ChainSettings& chainSettings);
    void updateHighCutFilter(const ChainSettings& chainSettings);

    void updateFilter();

    

    //let's make another template to reduce code in switch below
   
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompASAudioProcessor)
};
