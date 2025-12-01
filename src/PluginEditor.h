#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PlaitsVSTEditor : public juce::AudioProcessorEditor,
                        public juce::Timer
{
public:
    explicit PlaitsVSTEditor(PlaitsVSTProcessor&);
    ~PlaitsVSTEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // Timer callback for UI refresh
    void timerCallback() override;

private:
    // Row types: Preset is special, others map to parameters
    enum class RowType { Preset, Engine, Harmonics, Timbre, Morph, Attack, Decay, Voices };
    static constexpr int kNumRows = 8;

    struct RowConfig {
        const char* label;
        RowType type;
        int minVal;
        int maxVal;
        int smallStep;
        int largeStep;
        const char* suffix;
    };

    static const RowConfig kRowConfigs[kNumRows];

    // Get/set values in display units
    int getDisplayValue(int row) const;
    void setDisplayValue(int row, int value);
    juce::String formatValue(int row) const;
    float normalizedValue(int row) const;

    // Navigation
    void adjustValue(int row, int delta);
    int rowAtY(int y) const;

    PlaitsVSTProcessor& processor_;
    int selectedRow_ = 0;
    int dragStartValue_ = 0;
    int dragStartX_ = 0;

    // Engine names for display
    const juce::StringArray engineNames_ = {
        "VA", "WAVSHP", "FM", "GRAIN", "ADDTIV", "WAVTBL",
        "CHORD", "SPEECH", "SWARM", "NOISE", "PARTCL", "STRING",
        "MODAL", "B.DRUM", "SNARE", "HI-HAT"
    };

    // Dynamic parameter labels per engine [engine][0=harmonics, 1=timbre, 2=morph]
    static const char* const kEngineParamLabels[16][3];

    // Get dynamic label for a row based on current engine
    const char* getDynamicLabel(int row) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaitsVSTEditor)
};
