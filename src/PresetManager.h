#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <random>

class PlaitsVSTProcessor;

class PresetManager
{
public:
    struct Preset {
        juce::String name;
        juce::File file;
        bool isFactory;
        juce::ValueTree state;
    };

    explicit PresetManager(PlaitsVSTProcessor& processor);

    void initialize();

    // Preset access
    int getNumPresets() const { return static_cast<int>(presets_.size()); }
    const Preset& getPreset(int index) const { return presets_[static_cast<size_t>(index)]; }
    int getCurrentPresetIndex() const { return currentPresetIndex_; }

    // Navigation
    void loadPreset(int index);
    void nextPreset();
    void previousPreset();

    // Saving
    void saveCurrentAsNewPreset();

    // Modified state
    bool isModified() const { return isModified_; }
    void markModified() { isModified_ = true; }

    // Get current preset name for display
    juce::String getCurrentPresetName() const;

private:
    void loadFactoryPresets();
    void loadUserPresets();
    void rebuildPresetList();
    juce::String generateRandomName();
    juce::File getUserPresetsFolder();

    Preset loadPresetFromFile(const juce::File& file, bool isFactory);
    void applyPresetToProcessor(const Preset& preset);
    juce::ValueTree captureCurrentState();

    PlaitsVSTProcessor& processor_;
    std::vector<Preset> factoryPresets_;
    std::vector<Preset> userPresets_;
    std::vector<Preset> presets_;  // Merged and sorted
    int currentPresetIndex_ = 0;
    bool isModified_ = false;

    std::mt19937 rng_;

    static const juce::StringArray kAdjectives;
    static const juce::StringArray kNouns;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
