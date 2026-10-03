#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "LSAFlangerCore.h"

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
        l.add(std::make_unique<F>("rate",     "Rate",        R(0.02f, 8.f, 0.001f, 0.4f), 0.25f));
        l.add(std::make_unique<F>("depth",    "Depth",       R(0.f, 1.f), 0.7f));
        l.add(std::make_unique<F>("delay",    "Delay ms",    R(0.1f, 10.f, 0.01f, 0.5f), 1.0f));
        l.add(std::make_unique<F>("feedback", "Feedback",    R(-0.95f, 0.95f), 0.75f));
        l.add(std::make_unique<F>("drive",    "Metal Drive", R(0.f, 1.f), 0.5f));
        l.add(std::make_unique<F>("tone",     "Tone",        R(0.f, 1.f), 0.7f));
        l.add(std::make_unique<F>("shape",    "Sine/Tri",    R(0.f, 1.f), 0.f));
        l.add(std::make_unique<F>("stereo",   "Stereo",      R(0.f, 0.5f), 0.25f));
        l.add(std::make_unique<F>("mix",      "Mix",         R(0.f, 1.f), 0.5f));
        l.add(std::make_unique<F>("panrate",  "Pan Rate",    R(0.02f, 2.f, 0.001f, 0.5f), 0.15f));
        l.add(std::make_unique<F>("pandepth", "Pan Depth",   R(0.f, 1.f), 0.8f));
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
        auto get = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
        lsa::Params p;
        p.rateHz = get("rate"); p.depth = get("depth"); p.baseMs = get("delay");
        p.feedback = get("feedback"); p.drive = get("drive"); p.tone = get("tone");
        p.shape = get("shape"); p.stereo = get("stereo"); p.mix = get("mix");
        p.panRateHz = get("panrate"); p.panDepth = get("pandepth");
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

// Custom dark "metal" editor with rotary knobs.
class LSAFlangerEditor : public juce::AudioProcessorEditor
{
public:
    explicit LSAFlangerEditor(LSAFlangerProcessor& p) : AudioProcessorEditor(p)
    {
        const char* ids[]   = { "rate","depth","delay","feedback","drive","tone","shape","stereo","mix","panrate","pandepth" };
        const char* names[] = { "RATE","DEPTH","DELAY","FEEDBACK","METAL","TONE","SINE/TRI","STEREO","MIX","PAN RATE","PAN DEPTH" };
        for (int i = 0; i < 11; ++i)
        {
            auto k = std::make_unique<Knob>();
            k->s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            k->s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
            k->s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff3b30));
            k->s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe8e8e8));
            k->s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            k->s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            k->label.setText(names[i], juce::dontSendNotification);
            k->label.setJustificationType(juce::Justification::centred);
            k->label.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
            k->label.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, ids[i], k->s);
            addAndMakeVisible(k->s); addAndMakeVisible(k->label);
            knobs.push_back(std::move(k));
        }
        setSize(700, 400);
    }
    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c1c20), 0, 0, juce::Colour(0xff08080a), 0, (float) getHeight(), false));
        g.fillAll();
        g.setColour(juce::Colour(0xffff3b30));
        g.setFont(juce::FontOptions(30.0f, juce::Font::bold));
        g.drawText("LSA FLANGER", 20, 10, 400, 40, juce::Justification::left);
        g.setColour(juce::Colour(0xff808080));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("FLANGER", 20, 70, 300, 16, juce::Justification::left);
        g.drawText("OUTPUT / AUTO-PAN", 20, 235, 300, 16, juce::Justification::left);
    }
    void resized() override
    {
        // row 1: rate depth delay feedback metal tone ; row 2: sine/tri stereo mix panrate pandepth
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            int row = i < 6 ? 0 : 1, col = i < 6 ? (int) i : (int) i - 6;
            int x = 20 + col * 110, y = row == 0 ? 90 : 255;
            knobs[i]->label.setBounds(x, y, 100, 16);
            knobs[i]->s.setBounds(x, y + 16, 100, 100);
        }
    }
private:
    struct Knob { juce::Slider s; juce::Label label; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; };
    std::vector<std::unique_ptr<Knob>> knobs;
};

juce::AudioProcessorEditor* LSAFlangerProcessor::createEditor() { return new LSAFlangerEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LSAFlangerProcessor(); }
