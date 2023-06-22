/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CompASAudioProcessor::CompASAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

CompASAudioProcessor::~CompASAudioProcessor()
{
}

//==============================================================================
const juce::String CompASAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompASAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CompASAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CompASAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CompASAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompASAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CompASAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompASAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CompASAudioProcessor::getProgramName (int index)
{
    return {};
}

void CompASAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CompASAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //prepare filter before using by passing a process spec, it passes to each chain

    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    //max samples to process

    spec.numChannels = 1;
    //no of channels, mono=1

    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts); //we can get values for all our parameters

    updateFilter();

    //prepare fifo 
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);


   // updatePeakFilter(chainSettings);

    //this was for peak

   
    //to get lowCut and highCut filter, we invoke this function
    //for order = n, we get n/2 filters by the function
    // we need an array of coefficients for the band

    //slope choices 0,1,2,3
    //we have values 12, 24, 36, 48
    //so we use orders 2,4,6,8 because n order gives n/2 filter and each filter
    //gives a 12dB/oct value


    //refactored

    /*auto cutCoeff = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::lowCut>();
    updateCutFilter(leftLowCut, cutCoeff, chainSettings.lowCutSlope);
    auto& rightLowCut = rightChain.get<ChainPositions::lowCut>();
    updateCutFilter(rightLowCut, cutCoeff, chainSettings.lowCutSlope);*/


    /*auto cutCoeffH = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::highCut>();
    updateCutFilter(leftHighCut, cutCoeffH, chainSettings.highCutSlope);
    auto& rightHighCut = rightChain.get<ChainPositions::highCut>();
    updateCutFilter(rightHighCut, cutCoeffH, chainSettings.highCutSlope);*/

}

void CompASAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CompASAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void CompASAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // for our sliders to change values, we need to pass it before processing the blocks, so our sliders actually change values

    auto chainSettings = getChainSettings(apvts); //we can get values for all our parameters
    
    //updatePeakFilter(chainSettings);

    //post refactoring
    

    /*auto cutCoeff = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::lowCut>();

    updateCutFilter(leftLowCut, cutCoeff, chainSettings.lowCutSlope);
*/


    //leftLowCut.setBypassed<0>(true);
    //leftLowCut.setBypassed<1>(true);
    //leftLowCut.setBypassed<2>(true);
    //leftLowCut.setBypassed<3>(true);

    //switch (chainSettings.lowCutSlope) {
    //case Slope_12:
    //{
    //    //get the coefficient
    //    *leftLowCut.get<0>().coefficients = *cutCoeff[0];
    //    //stop bypassing
    //    leftLowCut.setBypassed<0>(false);
    //    break;
    //}
    //case Slope_24:
    //{
    //    *leftLowCut.get<0>().coefficients = *cutCoeff[0];
    //    leftLowCut.setBypassed<0>(false);
    //    *leftLowCut.get<1>().coefficients = *cutCoeff[1];
    //    leftLowCut.setBypassed<1>(false);

    //    break;
    //}
    //case Slope_36:
    //{
    //    *leftLowCut.get<0>().coefficients = *cutCoeff[0];
    //    leftLowCut.setBypassed<0>(false);
    //    *leftLowCut.get<1>().coefficients = *cutCoeff[1];
    //    leftLowCut.setBypassed<1>(false);
    //    *leftLowCut.get<2>().coefficients = *cutCoeff[2];
    //    leftLowCut.setBypassed<2>(false);
    //    break;
    //}
    //case Slope_48:
    //{
    //    *leftLowCut.get<0>().coefficients = *cutCoeff[0];
    //    leftLowCut.setBypassed<0>(false);
    //    *leftLowCut.get<1>().coefficients = *cutCoeff[1];
    //    leftLowCut.setBypassed<1>(false);
    //    *leftLowCut.get<2>().coefficients = *cutCoeff[2];
    //    leftLowCut.setBypassed<2>(false);
    //    *leftLowCut.get<3>().coefficients = *cutCoeff[3];
    //    leftLowCut.setBypassed<3>(false);
    //    break;
    //}

    //}

    // We can comment this and use same function
    // 
    //
    //auto& rightLowCut = rightChain.get<ChainPositions::lowCut>();
    //updateCutFilter(rightLowCut, cutCoeff, chainSettings.lowCutSlope);

    //rightLowCut.setBypassed<0>(true);
    //rightLowCut.setBypassed<1>(true);
    //rightLowCut.setBypassed<2>(true);
    //rightLowCut.setBypassed<3>(true);

    //switch (chainSettings.lowCutSlope) {
    //case Slope_12:
    //{
    //    //get the coefficient
    //    *rightLowCut.get<0>().coefficients = *cutCoeff[0];
    //    //stop bypassing
    //    rightLowCut.setBypassed<0>(false);
    //    break;
    //}
    //case Slope_24:
    //{
    //    *rightLowCut.get<0>().coefficients = *cutCoeff[0];
    //    rightLowCut.setBypassed<0>(false);
    //    *rightLowCut.get<1>().coefficients = *cutCoeff[1];
    //    rightLowCut.setBypassed<1>(false);

    //    break;
    //}
    //case Slope_36:
    //{
    //    *rightLowCut.get<0>().coefficients = *cutCoeff[0];
    //    rightLowCut.setBypassed<0>(false);
    //    *rightLowCut.get<1>().coefficients = *cutCoeff[1];
    //    rightLowCut.setBypassed<1>(false);
    //    *rightLowCut.get<2>().coefficients = *cutCoeff[2];
    //    rightLowCut.setBypassed<2>(false);
    //    break;
    //}
    //case Slope_48:
    //{
    //    *rightLowCut.get<0>().coefficients = *cutCoeff[0];
    //    rightLowCut.setBypassed<0>(false);
    //    *rightLowCut.get<1>().coefficients = *cutCoeff[1];
    //    rightLowCut.setBypassed<1>(false);
    //    *rightLowCut.get<2>().coefficients = *cutCoeff[2];
    //    rightLowCut.setBypassed<2>(false);
    //    *rightLowCut.get<3>().coefficients = *cutCoeff[3];
    //    rightLowCut.setBypassed<3>(false);
    //    break;
    //}

    //}

   // auto cutCoeffH = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, getSampleRate(), 2 * (chainSettings.highCutSlope + 1));

   // auto& leftHighCut = leftChain.get<ChainPositions::highCut>();
    //updateCutFilter(leftHighCut, cutCoeffH, chainSettings.highCutSlope);
   // auto& rightHighCut = rightChain.get<ChainPositions::highCut>();
   // updateCutFilter(rightHighCut, cutCoeffH, chainSettings.highCutSlope);

    updateFilter();

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    //after getting left and right, we get the context

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    // we can pass the context, now our plugin is getting audio

    //update fifo 
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);

}

//==============================================================================
bool CompASAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CompASAudioProcessor::createEditor()
{
  return new CompASAudioProcessorEditor (*this);
   //return new juce::GenericAudioProcessorEditor(*this);
    
}

//==============================================================================
void CompASAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    
    //implementing function to save the state of VST for the next time it is loaded, it is provided by apvts member -> state
    
    //output stream to destination, and true to append to existsing data
    juce::MemoryOutputStream mos(destData, true);
    //saving...
    apvts.state.writeToStream(mos);

}

void CompASAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    //to retrieve existing information
    //check whether tree state is valid

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateFilter();
    }
}



//populate our DS ChainSettings

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    //we can get parameters either by apvts.getParameter()->getValue
    //but we get normalized values

    //second way, we get raw values
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();

        return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void CompASAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings) {
/*    auto peakCoeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));  *///decibels class converts our decible value to gain

    //our coeff object is referenced to heap, so to get value we need to dereference
    
    // commenting cause refactored so function handles replacement
   // *leftChain.get<ChainPositions::peak>().coefficients = *peakCoeff;
    // *rightChain.get<ChainPositions::peak>().coefficients = *peakCoeff;
    auto peakCoeff = makePeakFilter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::peak>().coefficients, peakCoeff);
    updateCoefficients(rightChain.get<ChainPositions::peak>().coefficients, peakCoeff);

}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    *old = *replacements;
}

void CompASAudioProcessor::updateLowCutFilter(const ChainSettings& chainSettings) {
    auto cutCoeff = makeLowCutFilter(chainSettings, getSampleRate());

    auto& leftLowCut = leftChain.get<ChainPositions::lowCut>();
    updateCutFilter(leftLowCut, cutCoeff, chainSettings.lowCutSlope);
    auto& rightLowCut = rightChain.get<ChainPositions::lowCut>();
    updateCutFilter(rightLowCut, cutCoeff, chainSettings.lowCutSlope);
}

void CompASAudioProcessor::updateHighCutFilter(const ChainSettings& chainSettings) {
    auto cutCoeffH = makeHighCutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get<ChainPositions::highCut>();
    updateCutFilter(leftHighCut, cutCoeffH, chainSettings.highCutSlope);
    auto& rightHighCut = rightChain.get<ChainPositions::highCut>();
    updateCutFilter(rightHighCut, cutCoeffH, chainSettings.highCutSlope);
}

void CompASAudioProcessor::updateFilter() {
    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);
    updateHighCutFilter(chainSettings);
    updateLowCutFilter(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout
CompASAudioProcessor::createParameterLayout() 
{
    
    // using AudioParameterFloat means we're using a range of values
    juce::AudioProcessorValueTreeState::ParameterLayout layout; //make an object of class parameter layout
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", "LowCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
       //layout.add adds the parameter, we pass the name, ID, the min value and the default value, default for low pass=20Hz and highpass=20k Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq", "HighCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

    //peak freq
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));

    //peak gain(defined in decibles 
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));

    //quality control (low Q - wide) (high Q- narrow)
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.0f, 0.5f, 1.0f), 1.0f));

    //make a string array for parameters that accept in dB/Octave
    //bands like these use multiples of 12 or 6 so we fill it with multiples of 12
    juce::StringArray stringArray;
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i * 12);
        str << " dB/Oct";
        stringArray.add(str);
    }

    //now we add to layout
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompASAudioProcessor();
}
