#include <juce_audio_processors/juce_audio_processors.h>
#include "MetalFlangerCore.h"

class MetalFlangerProcessor : public juce::AudioProcessor
{
public:
    MetalFlangerProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "STATE", layout()) {}

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using F = juce::AudioParameterFloat;
        using R = juce::NormalisableRange<float>;
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add(std::make_unique<F>("rate",     "Flanger Rate Hz", R(0.02f, 5.f, 0.01f, 0.5f), 0.25f));
        l.add(std::make_unique<F>("depth",    "Depth",           R(0.f, 1.f), 0.7f));
        l.add(std::make_unique<F>("feedback", "Feedback",        R(-0.95f, 0.95f), 0.75f));
        l.add(std::make_unique<F>("drive",    "Metal Drive",     R(0.f, 1.f), 0.5f));
        l.add(std::make_unique<F>("mix",      "Mix",             R(0.f, 1.f), 0.5f));
        l.add(std::make_unique<F>("panrate",  "Pan Rate Hz",     R(0.02f, 2.f, 0.01f, 0.5f), 0.15f));
        l.add(std::make_unique<F>("pandepth", "Pan Depth",       R(0.f, 1.f), 0.8f));
        return l;
    }

    void prepareToPlay(double sr, int) override { core.prepare(sr); }
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        MetalFlangerCore::Params p;
        p.rateHz = *apvts.getRawParameterValue("rate");
        p.depth = *apvts.getRawParameterValue("depth");
        p.feedback = *apvts.getRawParameterValue("feedback");
        p.drive = *apvts.getRawParameterValue("drive");
        p.mix = *apvts.getRawParameterValue("mix");
        p.panRateHz = *apvts.getRawParameterValue("panrate");
        p.panDepth = *apvts.getRawParameterValue("pandepth");
        auto* L = b.getWritePointer(0);
        auto* R = b.getNumChannels() > 1 ? b.getWritePointer(1) : b.getWritePointer(0);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float l = L[i], r = R[i];
            core.process(l, r, p);
            L[i] = l; R[i] = r;
        }
    }

    juce::AudioProcessorEditor* createEditor() override
    { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Metal Flanger"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& d) override
    { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
    void setStateInformation(const void* d, int s) override
    { if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }

private:
    juce::AudioProcessorValueTreeState apvts;
    MetalFlangerCore core;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalFlangerProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MetalFlangerProcessor(); }
