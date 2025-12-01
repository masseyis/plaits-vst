#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/voice_allocator.h"

class PresetManager;

class PlaitsVSTProcessor : public juce::AudioProcessor
{
public:
    PlaitsVSTProcessor();
    ~PlaitsVSTProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter accessors
    juce::AudioParameterChoice* getEngineParam() { return engineParam_; }
    juce::AudioParameterFloat* getHarmonicsParam() { return harmonicsParam_; }
    juce::AudioParameterFloat* getTimbreParam() { return timbreParam_; }
    juce::AudioParameterFloat* getMorphParam() { return morphParam_; }
    juce::AudioParameterFloat* getAttackParam() { return attackParam_; }
    juce::AudioParameterFloat* getDecayParam() { return decayParam_; }
    juce::AudioParameterInt* getPolyphonyParam() { return polyphonyParam_; }

    // Preset manager
    PresetManager& getPresetManager();

private:
    void handleMidiMessage(const juce::MidiMessage& msg);

    VoiceAllocator voiceAllocator_;
    double hostSampleRate_ = 44100.0;

    // Parameters
    juce::AudioParameterChoice* engineParam_ = nullptr;
    juce::AudioParameterFloat* harmonicsParam_ = nullptr;
    juce::AudioParameterFloat* timbreParam_ = nullptr;
    juce::AudioParameterFloat* morphParam_ = nullptr;
    juce::AudioParameterFloat* attackParam_ = nullptr;
    juce::AudioParameterFloat* decayParam_ = nullptr;
    juce::AudioParameterInt* polyphonyParam_ = nullptr;

    // Preset manager
    std::unique_ptr<PresetManager> presetManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaitsVSTProcessor)
};
