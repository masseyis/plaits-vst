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

    const juce::StringArray lfoRateNames = {
        "1/16", "1/8", "1/4", "1/2", "1BAR", "2BAR", "4BAR"
    };

    const juce::StringArray lfoShapeNames = {
        "TRI", "SIN", "SAW", "SQR", "S&H"
    };

    const juce::StringArray modDestNames = {
        "HARMNIC", "TIMBRE", "MORPH", "CUTOFF", "RESONAN", "LFO1 RT", "LFO1 AM", "LFO2 RT", "LFO2 AM"
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

    // Filter parameters
    addParameter(cutoffParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("cutoff", 1),
        "Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f  // Fully open by default
    ));

    addParameter(resonanceParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("resonance", 1),
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f  // No resonance by default
    ));

    // LFO1 parameters
    addParameter(lfo1RateParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1rate", 1),
        "LFO1 Rate",
        lfoRateNames,
        2  // 1/4 default
    ));

    addParameter(lfo1ShapeParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1shape", 1),
        "LFO1 Shape",
        lfoShapeNames,
        0  // TRI default
    ));

    addParameter(lfo1DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1dest", 1),
        "LFO1 Dest",
        modDestNames,
        1  // TIMBRE default
    ));

    addParameter(lfo1AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("lfo1amount", 1),
        "LFO1 Amount",
        -64, 63, 0  // Bipolar, off by default
    ));

    // LFO2 parameters
    addParameter(lfo2RateParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2rate", 1),
        "LFO2 Rate",
        lfoRateNames,
        3  // 1/2 default
    ));

    addParameter(lfo2ShapeParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2shape", 1),
        "LFO2 Shape",
        lfoShapeNames,
        1  // SIN default
    ));

    addParameter(lfo2DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2dest", 1),
        "LFO2 Dest",
        modDestNames,
        2  // MORPH default
    ));

    addParameter(lfo2AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("lfo2amount", 1),
        "LFO2 Amount",
        -64, 63, 0
    ));

    // ENV1 parameters
    addParameter(env1AttackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env1attack", 1),
        "ENV1 Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f
    ));

    addParameter(env1DecayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env1decay", 1),
        "ENV1 Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f
    ));

    addParameter(env1DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("env1dest", 1),
        "ENV1 Dest",
        modDestNames,
        0  // HARMONICS default
    ));

    addParameter(env1AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("env1amount", 1),
        "ENV1 Amount",
        -64, 63, 0
    ));

    // ENV2 parameters
    addParameter(env2AttackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env2attack", 1),
        "ENV2 Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f
    ));

    addParameter(env2DecayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env2decay", 1),
        "ENV2 Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    addParameter(env2DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("env2dest", 1),
        "ENV2 Dest",
        modDestNames,
        3  // CUTOFF default
    ));

    addParameter(env2AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("env2amount", 1),
        "ENV2 Amount",
        -64, 63, 0
    ));

    voiceAllocator_.Init(44100.0, 8);

    // Initialize modulation matrix and filter
    modMatrix_.Init();
    filter_.Init(44100.0f);

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
    filter_.Init(static_cast<float>(sampleRate));
    modMatrix_.Reset();
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

    // Track voices for envelope triggering
    int prevActiveVoices = activeVoiceCount_;

    // Handle MIDI messages
    for (const auto metadata : midiMessages)
    {
        handleMidiMessage(metadata.getMessage());
    }

    // Update polyphony if changed
    voiceAllocator_.setPolyphony(polyphonyParam_->get());

    // Update modulation parameters from UI
    updateModulationParams();

    // Process modulation matrix
    const int numSamples = buffer.getNumSamples();
    modMatrix_.Process(static_cast<float>(hostSampleRate_), numSamples);

    // Trigger envelopes on first note after silence
    activeVoiceCount_ = voiceAllocator_.activeVoiceCount();
    if (activeVoiceCount_ > 0 && prevActiveVoices == 0) {
        modMatrix_.TriggerEnvelopes();
    }

    // Get modulated values
    float modulatedHarmonics = getModulatedHarmonics();
    float modulatedTimbre = getModulatedTimbre();
    float modulatedMorph = getModulatedMorph();
    float modulatedCutoff = getModulatedCutoff();
    float modulatedResonance = getModulatedResonance();

    // Update shared parameters with modulated values
    voiceAllocator_.set_engine(engineParam_->getIndex());
    voiceAllocator_.set_harmonics(modulatedHarmonics);
    voiceAllocator_.set_timbre(modulatedTimbre);
    voiceAllocator_.set_morph(modulatedMorph);

    // Update filter with modulated values
    // Map 0-1 to exponential frequency range (20Hz to 20kHz)
    float cutoffHz = 20.0f * std::pow(1000.0f, modulatedCutoff);
    filter_.SetCutoff(cutoffHz);
    filter_.SetResonance(modulatedResonance);

    // Get output pointers
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // Process all voices
    if (rightChannel) {
        voiceAllocator_.Process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    } else {
        // Mono output
        float tempRight[2048];
        size_t samples = std::min(static_cast<size_t>(numSamples), sizeof(tempRight) / sizeof(float));
        voiceAllocator_.Process(leftChannel, tempRight, samples);
    }

    // Apply filter to output
    for (int i = 0; i < numSamples; ++i) {
        leftChannel[i] = filter_.Process(leftChannel[i]);
        if (rightChannel) {
            rightChannel[i] = filter_.Process(rightChannel[i]);
        }
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

    // Filter params
    state.setProperty("cutoff", cutoffParam_->get(), nullptr);
    state.setProperty("resonance", resonanceParam_->get(), nullptr);

    // LFO1 params
    state.setProperty("lfo1rate", lfo1RateParam_->getIndex(), nullptr);
    state.setProperty("lfo1shape", lfo1ShapeParam_->getIndex(), nullptr);
    state.setProperty("lfo1dest", lfo1DestParam_->getIndex(), nullptr);
    state.setProperty("lfo1amount", lfo1AmountParam_->get(), nullptr);

    // LFO2 params
    state.setProperty("lfo2rate", lfo2RateParam_->getIndex(), nullptr);
    state.setProperty("lfo2shape", lfo2ShapeParam_->getIndex(), nullptr);
    state.setProperty("lfo2dest", lfo2DestParam_->getIndex(), nullptr);
    state.setProperty("lfo2amount", lfo2AmountParam_->get(), nullptr);

    // ENV1 params
    state.setProperty("env1attack", env1AttackParam_->get(), nullptr);
    state.setProperty("env1decay", env1DecayParam_->get(), nullptr);
    state.setProperty("env1dest", env1DestParam_->getIndex(), nullptr);
    state.setProperty("env1amount", env1AmountParam_->get(), nullptr);

    // ENV2 params
    state.setProperty("env2attack", env2AttackParam_->get(), nullptr);
    state.setProperty("env2decay", env2DecayParam_->get(), nullptr);
    state.setProperty("env2dest", env2DestParam_->getIndex(), nullptr);
    state.setProperty("env2amount", env2AmountParam_->get(), nullptr);

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

        // Filter params
        if (state.hasProperty("cutoff"))
            *cutoffParam_ = static_cast<float>(state.getProperty("cutoff"));
        if (state.hasProperty("resonance"))
            *resonanceParam_ = static_cast<float>(state.getProperty("resonance"));

        // LFO1 params
        if (state.hasProperty("lfo1rate"))
            *lfo1RateParam_ = static_cast<int>(state.getProperty("lfo1rate"));
        if (state.hasProperty("lfo1shape"))
            *lfo1ShapeParam_ = static_cast<int>(state.getProperty("lfo1shape"));
        if (state.hasProperty("lfo1dest"))
            *lfo1DestParam_ = static_cast<int>(state.getProperty("lfo1dest"));
        if (state.hasProperty("lfo1amount"))
            *lfo1AmountParam_ = static_cast<int>(state.getProperty("lfo1amount"));

        // LFO2 params
        if (state.hasProperty("lfo2rate"))
            *lfo2RateParam_ = static_cast<int>(state.getProperty("lfo2rate"));
        if (state.hasProperty("lfo2shape"))
            *lfo2ShapeParam_ = static_cast<int>(state.getProperty("lfo2shape"));
        if (state.hasProperty("lfo2dest"))
            *lfo2DestParam_ = static_cast<int>(state.getProperty("lfo2dest"));
        if (state.hasProperty("lfo2amount"))
            *lfo2AmountParam_ = static_cast<int>(state.getProperty("lfo2amount"));

        // ENV1 params
        if (state.hasProperty("env1attack"))
            *env1AttackParam_ = static_cast<float>(state.getProperty("env1attack"));
        if (state.hasProperty("env1decay"))
            *env1DecayParam_ = static_cast<float>(state.getProperty("env1decay"));
        if (state.hasProperty("env1dest"))
            *env1DestParam_ = static_cast<int>(state.getProperty("env1dest"));
        if (state.hasProperty("env1amount"))
            *env1AmountParam_ = static_cast<int>(state.getProperty("env1amount"));

        // ENV2 params
        if (state.hasProperty("env2attack"))
            *env2AttackParam_ = static_cast<float>(state.getProperty("env2attack"));
        if (state.hasProperty("env2decay"))
            *env2DecayParam_ = static_cast<float>(state.getProperty("env2decay"));
        if (state.hasProperty("env2dest"))
            *env2DestParam_ = static_cast<int>(state.getProperty("env2dest"));
        if (state.hasProperty("env2amount"))
            *env2AmountParam_ = static_cast<int>(state.getProperty("env2amount"));
    }
}

void PlaitsVSTProcessor::updateModulationParams()
{
    // LFO1
    modMatrix_.GetLfo1().SetRate(static_cast<plaits::LfoRateDivision>(lfo1RateParam_->getIndex()));
    modMatrix_.GetLfo1().SetShape(static_cast<plaits::LfoShape>(lfo1ShapeParam_->getIndex()));
    modMatrix_.SetDestination(plaits::ModSource::Lfo1,
                              static_cast<plaits::ModDestination>(lfo1DestParam_->getIndex()));
    modMatrix_.SetAmount(plaits::ModSource::Lfo1, static_cast<int8_t>(lfo1AmountParam_->get()));

    // LFO2
    modMatrix_.GetLfo2().SetRate(static_cast<plaits::LfoRateDivision>(lfo2RateParam_->getIndex()));
    modMatrix_.GetLfo2().SetShape(static_cast<plaits::LfoShape>(lfo2ShapeParam_->getIndex()));
    modMatrix_.SetDestination(plaits::ModSource::Lfo2,
                              static_cast<plaits::ModDestination>(lfo2DestParam_->getIndex()));
    modMatrix_.SetAmount(plaits::ModSource::Lfo2, static_cast<int8_t>(lfo2AmountParam_->get()));

    // ENV1 - map 0-1 to ms (0-500 attack, 10-2000 decay)
    float env1AttackMs = env1AttackParam_->get() * 500.0f;
    float env1DecayMs = 10.0f + env1DecayParam_->get() * 1990.0f;
    modMatrix_.GetEnv1().SetAttack(env1AttackMs);
    modMatrix_.GetEnv1().SetDecay(env1DecayMs);
    modMatrix_.SetDestination(plaits::ModSource::Env1,
                              static_cast<plaits::ModDestination>(env1DestParam_->getIndex()));
    modMatrix_.SetAmount(plaits::ModSource::Env1, static_cast<int8_t>(env1AmountParam_->get()));

    // ENV2
    float env2AttackMs = env2AttackParam_->get() * 500.0f;
    float env2DecayMs = 10.0f + env2DecayParam_->get() * 1990.0f;
    modMatrix_.GetEnv2().SetAttack(env2AttackMs);
    modMatrix_.GetEnv2().SetDecay(env2DecayMs);
    modMatrix_.SetDestination(plaits::ModSource::Env2,
                              static_cast<plaits::ModDestination>(env2DestParam_->getIndex()));
    modMatrix_.SetAmount(plaits::ModSource::Env2, static_cast<int8_t>(env2AmountParam_->get()));
}

float PlaitsVSTProcessor::getModulatedHarmonics() const
{
    return modMatrix_.GetModulatedValue(plaits::ModDestination::Harmonics, harmonicsParam_->get());
}

float PlaitsVSTProcessor::getModulatedTimbre() const
{
    return modMatrix_.GetModulatedValue(plaits::ModDestination::Timbre, timbreParam_->get());
}

float PlaitsVSTProcessor::getModulatedMorph() const
{
    return modMatrix_.GetModulatedValue(plaits::ModDestination::Morph, morphParam_->get());
}

float PlaitsVSTProcessor::getModulatedCutoff() const
{
    return modMatrix_.GetModulatedValue(plaits::ModDestination::Cutoff, cutoffParam_->get());
}

float PlaitsVSTProcessor::getModulatedResonance() const
{
    return modMatrix_.GetModulatedValue(plaits::ModDestination::Resonance, resonanceParam_->get());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlaitsVSTProcessor();
}
