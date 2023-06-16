/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//customized look and feel so we can rotate our slider in angles

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider &slider) 
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colours::lavender);
    g.fillEllipse(bounds);

    g.setColour(Colour(124u, 2u, 205u));
    g.drawEllipse(bounds, 2.f);

    //for text, we need to do a cast to call functions from juce string
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() *1.5);

        p.addRoundedRectangle(r, 2.f);

        //problem when jmapping
        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight()-3);
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 3, rswl->getTextHeight()+2);
        r.setCentre(bounds.getCentre());

      /*  g.setColour(Colours::black);
        g.fillRect(r);*/

        g.setColour(Colour(47u, 9u, 75u));
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centredBottom, 1);
    }

    
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    //set 7o'clock for start and 5 for end, i.e 7 represents 0 and 5 does full

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

  /*  g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);*/

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(47u, 9u, 75u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();

    for (int i = 0; i < numChoices; ++i) {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
  //  return getLocalBounds();
    auto bounds = getLocalBounds();

    //to make ellipses into circle
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    //let's make size for our fonts
    size -= getTextHeight() * 2;

    //move slider bounds towards top
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(),0);
    r.setY(2);

    return r;
}


juce::String RotarySliderWithLabels::getDisplayString() const {
    //let's use choice params for amending for slope

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    //now, if freq goes over 1k, we'll add string object to make it KHz

    juce::String str;
    bool addK = false;

    //check whether it is audio parameter
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
        float val = getValue();

        if (val > 999.f) {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0)); //if adding K, give 2 decimal places

    }
    else {
        jassertfalse;
    }

    if (suffix.isNotEmpty()) {
        str << " ";
        if (addK) str << "k";

        str << suffix;
    }
    return str;
}
//==============================================================================

ResponseCurveComponent::ResponseCurveComponent(CompASAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->addListener(this);
    }
    updateChain();
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent(){
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //update the monochain
        updateChain();
        //signal the repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain() {
    //update the monochain
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(MonoChain.get<ChainPositions::peak>().coefficients, peakCoefficients);

    auto lowcutCoeff = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highcutCoeff = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(MonoChain.get<ChainPositions::lowCut>(), lowcutCoeff, chainSettings.lowCutSlope);
    updateCutFilter(MonoChain.get<ChainPositions::highCut>(), highcutCoeff, chainSettings.highCutSlope);

}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    using namespace juce;
    g.fillAll(Colours::lavender);

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();

    auto& lowcut = MonoChain.get<ChainPositions::lowCut>();
    auto& peak = MonoChain.get<ChainPositions::peak>();
    auto& highcut = MonoChain.get<ChainPositions::highCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    //mags=magnitude, we get magnitude from frequency, we'll store 
    //in vector

    //store one magnitude per pixel as per our width

    std::vector<double> mags;

    mags.resize(w);

    for (int i = 0; i < w; ++i) {
        double mag = 1.f; //these are expressed in gain units

        //we use mapToLog10 to get map from pixel->frequency for human hearing range
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        //we update magnitude 
        //if band is bypassed, then ignore
        if (!MonoChain.isBypassed<ChainPositions::peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        //since we have 4 low-cut possible values, we need for all four
        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        // same for highcut

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    //convert mag->path

    //build the path and map decible value to response area

    Path responseCurve;

    //maximum and minimum positions in window
    //jmap normalizes our values for mapping, remaps value between 0 and 1 to our given range
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input) {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    //create lines for every other magnitude
    for (size_t i = 1; i < mags.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::powderblue);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::royalblue);
    g.strokePath(responseCurve, PathStrokeType(3.f));

}



//add all sliders attachment in the constructor, this provides save state functionality alongside connecting dsp w slider
CompASAudioProcessorEditor::CompASAudioProcessorEditor(CompASAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.


    //add labels
    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });

    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "+24dB" });

    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });

    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });

    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });

    lowCutSlopeSlider.labels.add({ 0.0f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });

    highCutSlopeSlider.labels.add({ 0.0f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    //add your component buttons
    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }


    setSize (600, 400);
}

//==============================================================================
void CompASAudioProcessorEditor::paint (juce::Graphics& g)
{
        // (Our component is opaque, so we must completely fill the background with a solid colour)
    using namespace juce;
    g.fillAll (Colours::lavender);

}

void CompASAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    //juce live constant lets you place the components in real time


    //bound area
    auto bounds = getLocalBounds();

    float hRatio = 30.f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f;
    //remove 33% top
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);
    bounds.removeFromTop(5);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth()*0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth()*0.5);



    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()*0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}



std::vector<juce::Component*> CompASAudioProcessorEditor::getComps() {
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}
