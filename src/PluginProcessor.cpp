// PlaitsVST - Mutable Instruments Plaits port
// Original Plaits: Copyright 2016 Emilie Gillet (MIT License)
// PlaitsVST: MIT License

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetManager.h"

namespace {
    const juce::StringArray engineNames = {
        "VA",               // Virtual Analog
        "Waveshaper",       // Waveshaping
        "FM",               // 2-operator FM
        "Grain",            // Granular
        "Additive",         // Additive
        "Wavetable",        // Wavetable
        "Chord",            // Chord
        "Speech",           // Speech synthesis
        "Swarm",            // Swarm oscillator
        "Noise",            // Filtered noise
        "Particle",         // Particle noise
        "String",           // Karplus-Strong string
        "Modal",            // Modal resonator
        "Bass Drum",        // Analog bass drum
        "Snare",            // Analog snare drum
        "Hi-Hat"            // Analog hi-hat
    };
}

PlaitsVSTProcessor::PlaitsVSTProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Create parameters
    addParameter(engineParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("engine", 1),
        "Engine",
        engineNames,
        0  // Default to VA
    ));

    addParameter(harmonicsParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("harmonics", 1),
        "Harmonics",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    addParameter(timbreParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("timbre", 1),
        "Timbre",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    addParameter(morphParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("morph", 1),
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    // Attack: 0-500ms mapped to 0-1
    addParameter(attackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("attack", 1),
        "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f  // 50ms default
    ));

    // Decay: 10-2000ms mapped to 0-1
    addParameter(decayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("decay", 1),
        "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.095f  // ~200ms default
    ));

    addParameter(polyphonyParam_ = new juce::AudioParameterInt(
        juce::ParameterID("polyphony", 1),
        "Polyphony",
        1, 16, 8  // min, max, default
    ));

    voiceAllocator_.Init(44100.0, 8);

    // Initialize preset manager after parameters are created
    presetManager_ = std::make_unique<PresetManager>(*this);
    presetManager_->initialize();
}

PlaitsVSTProcessor::~PlaitsVSTProcessor() = default;

PresetManager& PlaitsVSTProcessor::getPresetManager()
{
    return *presetManager_;
}

void PlaitsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
    voiceAllocator_.Init(sampleRate, polyphonyParam_->get());
}

void PlaitsVSTProcessor::releaseResources()
{
}

void PlaitsVSTProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        // Convert attack/decay from 0-1 to ms
        float attackMs = attackParam_->get() * 500.0f;
        float decayMs = 10.0f + decayParam_->get() * 1990.0f;
        voiceAllocator_.NoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), attackMs, decayMs);
    }
    else if (msg.isNoteOff())
    {
        voiceAllocator_.NoteOff(msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        voiceAllocator_.AllNotesOff();
    }
}

void PlaitsVSTProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Handle MIDI messages
    for (const auto metadata : midiMessages)
    {
        handleMidiMessage(metadata.getMessage());
    }

    // Update polyphony if changed
    voiceAllocator_.setPolyphony(polyphonyParam_->get());

    // Update shared parameters
    voiceAllocator_.set_engine(engineParam_->getIndex());
    voiceAllocator_.set_harmonics(harmonicsParam_->get());
    voiceAllocator_.set_timbre(timbreParam_->get());
    voiceAllocator_.set_morph(morphParam_->get());

    // Get output pointers
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    const int numSamples = buffer.getNumSamples();

    // Process all voices
    if (rightChannel) {
        voiceAllocator_.Process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    } else {
        // Mono output
        float tempRight[2048];
        size_t samples = std::min(static_cast<size_t>(numSamples), sizeof(tempRight) / sizeof(float));
        voiceAllocator_.Process(leftChannel, tempRight, samples);
    }
}

juce::AudioProcessorEditor* PlaitsVSTProcessor::createEditor()
{
    return new PlaitsVSTEditor(*this);
}

void PlaitsVSTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("PlaitsVSTState");
    state.setProperty("engine", engineParam_->getIndex(), nullptr);
    state.setProperty("harmonics", harmonicsParam_->get(), nullptr);
    state.setProperty("timbre", timbreParam_->get(), nullptr);
    state.setProperty("morph", morphParam_->get(), nullptr);
    state.setProperty("attack", attackParam_->get(), nullptr);
    state.setProperty("decay", decayParam_->get(), nullptr);
    state.setProperty("polyphony", polyphonyParam_->get(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void PlaitsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        if (state.hasProperty("engine"))
            *engineParam_ = static_cast<int>(state.getProperty("engine"));
        if (state.hasProperty("harmonics"))
            *harmonicsParam_ = static_cast<float>(state.getProperty("harmonics"));
        if (state.hasProperty("timbre"))
            *timbreParam_ = static_cast<float>(state.getProperty("timbre"));
        if (state.hasProperty("morph"))
            *morphParam_ = static_cast<float>(state.getProperty("morph"));
        if (state.hasProperty("attack"))
            *attackParam_ = static_cast<float>(state.getProperty("attack"));
        if (state.hasProperty("decay"))
            *decayParam_ = static_cast<float>(state.getProperty("decay"));
        if (state.hasProperty("polyphony"))
            *polyphonyParam_ = static_cast<int>(state.getProperty("polyphony"));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlaitsVSTProcessor();
}
