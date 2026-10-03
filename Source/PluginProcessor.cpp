#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "LSAFlangerMacro.h"

class LSAFlangerProcessor : public juce::AudioProcessor
{
public:
    LSAFlangerProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "STATE", layout()) {}

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using F = juce::AudioParameterFloat;
        using R = juce::NormalisableRange<float>;
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add(std::make_unique<F>("effect", "Effect", R(0.f, 1.f, 0.001f), 0.5f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + " %"; })
                .withValueFromStringFunction([](const juce::String& t) { return t.getFloatValue() / 100.f; })));
        return l;
    }

    void prepareToPlay(double sr, int) override { core.prepare(sr); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    { return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
          && (l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
           || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()); }

    void processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        const lsa::Params p = lsa::macroToParams(apvts.getRawParameterValue("effect")->load());
        const bool stereoIO = b.getNumChannels() > 1;
        auto* L = b.getWritePointer(0);
        auto* R = stereoIO ? b.getWritePointer(1) : nullptr;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float l = L[i], r = R ? R[i] : L[i];
            core.process(l, r, p);
            L[i] = l; if (R) R[i] = r;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LSA Flanger"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& d) override
    { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
    void setStateInformation(const void* d, int s) override
    { if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }

    juce::AudioProcessorValueTreeState apvts;
private:
    lsa::FlangerCore core;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LSAFlangerProcessor)
};

// One big knob: turn up for more effect, down for less (fully clean at 0).
class LSAFlangerEditor : public juce::AudioProcessorEditor
{
public:
    explicit LSAFlangerEditor(LSAFlangerProcessor& p) : AudioProcessorEditor(p)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 24);
        knob.setRotaryParameters(juce::degreesToRadians(225.f), juce::degreesToRadians(495.f), true);
        knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff3b30));
        knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a30));
        knob.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe8e8e8));
        knob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knob.textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 100.0)) + " %"; };
        knob.valueFromTextFunction = [](const juce::String& t) { return t.getDoubleValue() / 100.0; };
        knob.setDoubleClickReturnValue(true, 0.5);
        addAndMakeVisible(knob);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "effect", knob);
        setSize(320, 360);
    }
    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c1c20), 0, 0, juce::Colour(0xff08080a), 0, (float) getHeight(), false));
        g.fillAll();
        g.setColour(juce::Colour(0xffff3b30));
        g.setFont(juce::FontOptions(30.0f, juce::Font::bold));
        g.drawText("LSA FLANGER", getLocalBounds().removeFromTop(60), juce::Justification::centred);
        g.setColour(juce::Colour(0xff9a9aa0));
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("EFFECT", 0, 300, getWidth(), 20, juce::Justification::centred);
    }
    void resized() override { knob.setBounds(40, 70, getWidth() - 80, 230); }
private:
    juce::Slider knob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

juce::AudioProcessorEditor* LSAFlangerProcessor::createEditor() { return new LSAFlangerEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LSAFlangerProcessor(); }
