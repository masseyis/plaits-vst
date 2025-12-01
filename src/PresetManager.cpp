#include "PresetManager.h"
#include "PluginProcessor.h"

const juce::StringArray PresetManager::kAdjectives = {
    "Neon", "Chrome", "Void", "Plasma", "Quantum",
    "Cyber", "Binary", "Static", "Flux", "Dark"
};

const juce::StringArray PresetManager::kNouns = {
    "Grid", "Pulse", "Signal", "Wave", "Core",
    "Drone", "Circuit", "Glitch", "Beam", "Echo"
};

PresetManager::PresetManager(PlaitsVSTProcessor& processor)
    : processor_(processor)
    , rng_(static_cast<unsigned>(std::time(nullptr)))
{
}

void PresetManager::initialize()
{
    loadFactoryPresets();
    loadUserPresets();
    rebuildPresetList();

    if (!presets_.empty()) {
        loadPreset(0);
    }
}

juce::File PresetManager::getUserPresetsFolder()
{
    auto musicFolder = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
    return musicFolder.getChildFile("PlaitsVST").getChildFile("Presets");
}

void PresetManager::loadFactoryPresets()
{
    factoryPresets_.clear();

    // Factory presets for Plaits
    // Format: name, engine, harmonics, timbre, morph, attack, decay, voices
    struct FactoryPresetData {
        const char* name;
        int engine;
        float harmonics;
        float timbre;
        float morph;
        float attack;
        float decay;
        int voices;
    };

    const FactoryPresetData factoryData[] = {
        {"Init",           0,  0.500f, 0.500f, 0.500f, 0.100f, 0.095f, 8},  // VA
        {"Neon Lead",      0,  0.700f, 0.800f, 0.300f, 0.010f, 0.146f, 4},  // VA
        {"Chrome Bass",    0,  0.200f, 0.400f, 0.700f, 0.000f, 0.196f, 2},  // VA
        {"Void Pad",       5,  0.500f, 0.600f, 0.400f, 0.400f, 0.749f, 8},  // Wavetable
        {"Plasma Pluck",  11,  0.700f, 0.500f, 0.300f, 0.000f, 0.070f, 6},  // String
        {"Quantum Bell",  12,  0.800f, 0.300f, 0.600f, 0.000f, 0.397f, 8},  // Modal
        {"Cyber FM",       2,  0.550f, 0.700f, 0.400f, 0.020f, 0.121f, 4},  // FM
        {"Binary Grain",   3,  0.400f, 0.800f, 0.500f, 0.010f, 0.095f, 4},  // Grain
        {"Static Drone",   9,  0.300f, 0.500f, 0.600f, 0.600f, 1.000f, 6},  // Noise
        {"Flux Wave",      1,  0.650f, 0.350f, 0.500f, 0.040f, 0.246f, 4},  // Waveshaper
        {"Dark Swarm",     8,  0.250f, 0.900f, 0.700f, 0.100f, 0.296f, 4},  // Swarm
        {"Glitch Speech",  7,  1.000f, 1.000f, 0.500f, 0.000f, 0.045f, 4},  // Speech
        {"808 Kick",      13,  0.300f, 0.500f, 0.400f, 0.000f, 0.150f, 1},  // Bass Drum
        {"Snare Hit",     14,  0.500f, 0.600f, 0.500f, 0.000f, 0.100f, 1},  // Snare
        {"Hi-Hat Sizzle", 15,  0.700f, 0.400f, 0.600f, 0.000f, 0.050f, 1},  // Hi-Hat
        {"Chord Stab",     6,  0.500f, 0.700f, 0.400f, 0.005f, 0.200f, 4},  // Chord
    };

    for (const auto& data : factoryData) {
        Preset preset;
        preset.name = data.name;
        preset.isFactory = true;

        juce::ValueTree state("PlaitsVSTState");
        state.setProperty("name", data.name, nullptr);
        state.setProperty("engine", data.engine, nullptr);
        state.setProperty("harmonics", data.harmonics, nullptr);
        state.setProperty("timbre", data.timbre, nullptr);
        state.setProperty("morph", data.morph, nullptr);
        state.setProperty("attack", data.attack, nullptr);
        state.setProperty("decay", data.decay, nullptr);
        state.setProperty("polyphony", data.voices, nullptr);
        preset.state = state;

        factoryPresets_.push_back(preset);
    }
}

void PresetManager::loadUserPresets()
{
    userPresets_.clear();

    auto folder = getUserPresetsFolder();
    if (!folder.exists()) {
        return;
    }

    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files) {
        auto preset = loadPresetFromFile(file, false);
        if (preset.state.isValid()) {
            userPresets_.push_back(preset);
        }
    }
}

PresetManager::Preset PresetManager::loadPresetFromFile(const juce::File& file, bool isFactory)
{
    Preset preset;
    preset.file = file;
    preset.isFactory = isFactory;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr) {
        preset.state = juce::ValueTree::fromXml(*xml);
        if (preset.state.hasProperty("name")) {
            preset.name = preset.state.getProperty("name").toString();
        } else {
            preset.name = file.getFileNameWithoutExtension();
        }
    }

    return preset;
}

void PresetManager::rebuildPresetList()
{
    presets_.clear();

    // Add factory presets first
    for (const auto& p : factoryPresets_) {
        presets_.push_back(p);
    }

    // Add user presets
    for (const auto& p : userPresets_) {
        presets_.push_back(p);
    }

    // Sort: factory first, then alphabetically within each group
    std::sort(presets_.begin(), presets_.end(),
              [](const Preset& a, const Preset& b) {
                  if (a.isFactory != b.isFactory) {
                      return a.isFactory;
                  }
                  return a.name.compareIgnoreCase(b.name) < 0;
              });
}

void PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets_.size())) {
        return;
    }

    currentPresetIndex_ = index;
    applyPresetToProcessor(presets_[static_cast<size_t>(index)]);
    isModified_ = false;
}

void PresetManager::applyPresetToProcessor(const Preset& preset)
{
    const auto& state = preset.state;

    if (state.hasProperty("engine")) {
        *processor_.getEngineParam() = static_cast<int>(state.getProperty("engine"));
    }
    if (state.hasProperty("harmonics")) {
        *processor_.getHarmonicsParam() = static_cast<float>(state.getProperty("harmonics"));
    }
    if (state.hasProperty("timbre")) {
        *processor_.getTimbreParam() = static_cast<float>(state.getProperty("timbre"));
    }
    if (state.hasProperty("morph")) {
        *processor_.getMorphParam() = static_cast<float>(state.getProperty("morph"));
    }
    if (state.hasProperty("attack")) {
        *processor_.getAttackParam() = static_cast<float>(state.getProperty("attack"));
    }
    if (state.hasProperty("decay")) {
        *processor_.getDecayParam() = static_cast<float>(state.getProperty("decay"));
    }
    if (state.hasProperty("polyphony")) {
        *processor_.getPolyphonyParam() = static_cast<int>(state.getProperty("polyphony"));
    }
}

void PresetManager::nextPreset()
{
    if (presets_.empty()) return;
    loadPreset((currentPresetIndex_ + 1) % static_cast<int>(presets_.size()));
}

void PresetManager::previousPreset()
{
    if (presets_.empty()) return;
    int newIndex = currentPresetIndex_ - 1;
    if (newIndex < 0) {
        newIndex = static_cast<int>(presets_.size()) - 1;
    }
    loadPreset(newIndex);
}

juce::ValueTree PresetManager::captureCurrentState()
{
    juce::ValueTree state("PlaitsVSTState");
    state.setProperty("engine", processor_.getEngineParam()->getIndex(), nullptr);
    state.setProperty("harmonics", processor_.getHarmonicsParam()->get(), nullptr);
    state.setProperty("timbre", processor_.getTimbreParam()->get(), nullptr);
    state.setProperty("morph", processor_.getMorphParam()->get(), nullptr);
    state.setProperty("attack", processor_.getAttackParam()->get(), nullptr);
    state.setProperty("decay", processor_.getDecayParam()->get(), nullptr);
    state.setProperty("polyphony", processor_.getPolyphonyParam()->get(), nullptr);
    return state;
}

juce::String PresetManager::generateRandomName()
{
    std::uniform_int_distribution<int> adjDist(0, kAdjectives.size() - 1);
    std::uniform_int_distribution<int> nounDist(0, kNouns.size() - 1);

    return kAdjectives[adjDist(rng_)] + " " + kNouns[nounDist(rng_)];
}

void PresetManager::saveCurrentAsNewPreset()
{
    auto folder = getUserPresetsFolder();
    if (!folder.exists()) {
        folder.createDirectory();
    }

    // Generate a unique name
    juce::String baseName = generateRandomName();
    juce::String name = baseName;
    juce::File file = folder.getChildFile(name + ".xml");

    int suffix = 2;
    while (file.exists()) {
        name = baseName + " " + juce::String(suffix);
        file = folder.getChildFile(name + ".xml");
        suffix++;
    }

    // Capture current state
    auto state = captureCurrentState();
    state.setProperty("name", name, nullptr);

    // Write to file
    auto xml = state.createXml();
    if (xml != nullptr) {
        xml->writeTo(file);
    }

    // Add to user presets and rebuild
    Preset preset;
    preset.name = name;
    preset.file = file;
    preset.isFactory = false;
    preset.state = state;
    userPresets_.push_back(preset);

    rebuildPresetList();

    // Find and select the new preset
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i) {
        if (presets_[static_cast<size_t>(i)].name == name) {
            currentPresetIndex_ = i;
            break;
        }
    }

    isModified_ = false;
}

juce::String PresetManager::getCurrentPresetName() const
{
    if (presets_.empty()) {
        return "No Presets";
    }

    juce::String name = presets_[static_cast<size_t>(currentPresetIndex_)].name;
    if (isModified_) {
        name += " *";
    }
    return name;
}
