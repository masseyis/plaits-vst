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
    void parentHierarchyChanged() override;

    // Timer callback for UI refresh
    void timerCallback() override;

private:
    // Row types - compact layout with multi-field mod rows
    enum class RowType {
        Preset, Engine, Harmonics, Timbre, Morph, Attack, Decay, Voices,
        Cutoff, Resonance,
        Lfo1, Lfo2, Env1, Env2  // Multi-field rows
    };
    static constexpr int kNumRows = 14;

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

    // Multi-field support for mod rows (LFO1, LFO2, ENV1, ENV2)
    bool isModRow(int row) const;
    int getNumFieldsForRow(int row) const;
    int getModFieldValue(int row, int field) const;
    void setModFieldValue(int row, int field, int value);
    juce::String formatModFieldValue(int row, int field) const;
    int getLargeStepForModField(int row, int field) const;

    // Navigation
    void adjustValue(int row, int delta);
    int rowAtY(int y) const;

    // Check if row has modulation visualization
    bool hasModOverlay(int row) const;
    float getModOverlayValue(int row) const;

    // Painting helpers
    void paintModRow(juce::Graphics& g, int row, const juce::Rectangle<int>& rowRect, bool selected);
    void paintModulationOverlay(juce::Graphics& g, int row, const juce::Rectangle<int>& barRect);

    // Get dynamic label for a row based on current engine
    const char* getDynamicLabel(int row) const;

    // Get mod destination name with dynamic engine labels
    juce::String getModDestinationName(int destIndex) const;

    PlaitsVSTProcessor& processor_;
    int selectedRow_ = 0;
    int selectedField_ = 0;  // For multi-field mod rows
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaitsVSTEditor)
};
