#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "BobaCore.h"

class BobaAudioProcessor : public juce::AudioProcessor
{
public:
    BobaAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Boba"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Last `num` samples (mono mix) of the input or output, for the spectrum display (UI thread).
    static constexpr int kScopeSize = 4096;
    void copyScope (bool output, float* dest, int num) const;

private:
    std::array<std::atomic<float>, kScopeSize> scopeIn {}, scopeOut {};
    std::atomic<int> scopeWrite { 0 };

    boba::Settings readSettings() const;
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    boba::Processor core;
    double sampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BobaAudioProcessor)
};
