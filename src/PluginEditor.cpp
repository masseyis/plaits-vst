#include "PluginEditor.h"
#include "PresetManager.h"

// Dynamic parameter labels per engine [engine][0=harmonics, 1=timbre, 2=morph]
// Short labels that fit in the UI (max ~10 chars)
const char* const PlaitsVSTEditor::kEngineParamLabels[16][3] = {
    // 0: VA (Virtual Analog)
    {"DETUNE", "SQUARE", "SAW"},
    // 1: Waveshaper
    {"WAVEFORM", "FOLD", "ASYMMETRY"},
    // 2: FM (Phase Modulation)
    {"RATIO", "MOD INDEX", "FEEDBACK"},
    // 3: Grain (Formant)
    {"FORMANT 2", "FORMANT", "WIDTH"},
    // 4: Additive
    {"BUMPS", "HARMONIC", "SHAPE"},
    // 5: Wavetable
    {"BANK", "ROW", "COLUMN"},
    // 6: Chord
    {"CHORD", "INVERSION", "WAVEFORM"},
    // 7: Speech
    {"TYPE", "SPECIES", "PHONEME"},
    // 8: Swarm (Granular)
    {"PITCH RND", "DENSITY", "OVERLAP"},
    // 9: Noise (Filtered)
    {"FILTER", "CLOCK", "RESONANCE"},
    // 10: Particle (Dust)
    {"FREQ RND", "DENSITY", "REVERB"},
    // 11: String
    {"INHARM", "EXCITER", "DECAY"},
    // 12: Modal
    {"MATERIAL", "EXCITER", "DECAY"},
    // 13: Bass Drum
    {"ATTACK", "TONE", "DECAY"},
    // 14: Snare
    {"NOISE", "MODES", "DECAY"},
    // 15: Hi-Hat
    {"METAL", "HIGHPASS", "DECAY"},
};

// Row configuration: label, type, min, max, smallStep, largeStep, suffix
const PlaitsVSTEditor::RowConfig PlaitsVSTEditor::kRowConfigs[kNumRows] = {
    {"PRESET",    RowType::Preset,    0,   0,    1,   1,  ""},   // min/max set dynamically
    {"ENGINE",    RowType::Engine,    0,   15,   1,   1,  ""},
    {"HARMONICS", RowType::Harmonics, 0,   127,  1,   13, ""},
    {"TIMBRE",    RowType::Timbre,    0,   127,  1,   13, ""},
    {"MORPH",     RowType::Morph,     0,   127,  1,   13, ""},
    {"ATTACK",    RowType::Attack,    0,   500,  5,   50, "ms"},
    {"DECAY",     RowType::Decay,     10,  2000, 20,  200, "ms"},
    {"VOICES",    RowType::Voices,    1,   16,   1,   2,  ""},
};

namespace {
    // Colors (same teal theme as BraidsVST)
    const juce::Colour kBgColor(0xff1a1a1a);
    const juce::Colour kRowBgColor(0xff0d0d0d);
    const juce::Colour kBarColor(0xff4a9090);
    const juce::Colour kBarSelectedColor(0xff6abfbf);
    const juce::Colour kTextColor(0xffa0a0a0);
    const juce::Colour kTextSelectedColor(0xffffffff);
    const juce::Colour kTitleColor(0xffffffff);

    // Layout
    constexpr int kWindowWidth = 320;
    constexpr int kWindowHeight = 252;  // One more row than Braids
    constexpr int kTitleHeight = 32;
    constexpr int kRowHeight = 26;
    constexpr int kRowMargin = 2;
    constexpr int kLabelWidth = 80;
    constexpr int kValueWidth = 100;
    constexpr int kPadding = 8;
}

PlaitsVSTEditor::PlaitsVSTEditor(PlaitsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(kWindowWidth, kWindowHeight);
    setWantsKeyboardFocus(true);
    startTimerHz(30);  // Refresh UI at 30fps for modified indicator
}

PlaitsVSTEditor::~PlaitsVSTEditor()
{
    stopTimer();
}

void PlaitsVSTEditor::timerCallback()
{
    repaint();  // Refresh to show modified state and preset changes
}

int PlaitsVSTEditor::getDisplayValue(int row) const
{
    const auto& cfg = kRowConfigs[row];
    switch (cfg.type) {
        case RowType::Preset:
            return processor_.getPresetManager().getCurrentPresetIndex();
        case RowType::Engine:
            return processor_.getEngineParam()->getIndex();
        case RowType::Harmonics:
            return static_cast<int>(processor_.getHarmonicsParam()->get() * 127.0f + 0.5f);
        case RowType::Timbre:
            return static_cast<int>(processor_.getTimbreParam()->get() * 127.0f + 0.5f);
        case RowType::Morph:
            return static_cast<int>(processor_.getMorphParam()->get() * 127.0f + 0.5f);
        case RowType::Attack:
            return static_cast<int>(processor_.getAttackParam()->get() * 500.0f + 0.5f);
        case RowType::Decay:
            return static_cast<int>(processor_.getDecayParam()->get() * 1990.0f + 10.0f + 0.5f);
        case RowType::Voices:
            return processor_.getPolyphonyParam()->get();
    }
    return 0;
}

void PlaitsVSTEditor::setDisplayValue(int row, int value)
{
    const auto& cfg = kRowConfigs[row];

    switch (cfg.type) {
        case RowType::Preset: {
            int numPresets = processor_.getPresetManager().getNumPresets();
            if (numPresets > 0) {
                value = ((value % numPresets) + numPresets) % numPresets;
                processor_.getPresetManager().loadPreset(value);
            }
            break;
        }
        case RowType::Engine:
            value = juce::jlimit(0, 15, value);
            *processor_.getEngineParam() = value;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Harmonics:
            value = juce::jlimit(0, 127, value);
            *processor_.getHarmonicsParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Timbre:
            value = juce::jlimit(0, 127, value);
            *processor_.getTimbreParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Morph:
            value = juce::jlimit(0, 127, value);
            *processor_.getMorphParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Attack:
            value = juce::jlimit(0, 500, value);
            *processor_.getAttackParam() = value / 500.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Decay:
            value = juce::jlimit(10, 2000, value);
            *processor_.getDecayParam() = (value - 10.0f) / 1990.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Voices:
            value = juce::jlimit(1, 16, value);
            *processor_.getPolyphonyParam() = value;
            processor_.getPresetManager().markModified();
            break;
    }
    repaint();
}

juce::String PlaitsVSTEditor::formatValue(int row) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Preset) {
        return processor_.getPresetManager().getCurrentPresetName();
    }
    if (cfg.type == RowType::Engine) {
        int value = getDisplayValue(row);
        return engineNames_[value];
    }

    int value = getDisplayValue(row);
    juce::String str;

    if (cfg.type == RowType::Harmonics || cfg.type == RowType::Timbre || cfg.type == RowType::Morph) {
        str = juce::String(value).paddedLeft('0', 3);
    } else if (cfg.type == RowType::Voices) {
        str = juce::String(value).paddedLeft('0', 2);
    } else {
        str = juce::String(value) + cfg.suffix;
    }
    return str;
}

float PlaitsVSTEditor::normalizedValue(int row) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Preset) {
        int numPresets = processor_.getPresetManager().getNumPresets();
        if (numPresets <= 1) return 0.0f;
        int current = processor_.getPresetManager().getCurrentPresetIndex();
        return static_cast<float>(current) / static_cast<float>(numPresets - 1);
    }

    int value = getDisplayValue(row);
    return static_cast<float>(value - cfg.minVal) / static_cast<float>(cfg.maxVal - cfg.minVal);
}

const char* PlaitsVSTEditor::getDynamicLabel(int row) const
{
    const auto& cfg = kRowConfigs[row];

    // Only Harmonics, Timbre, and Morph have dynamic labels
    if (cfg.type != RowType::Harmonics && cfg.type != RowType::Timbre && cfg.type != RowType::Morph) {
        return cfg.label;
    }

    int engine = processor_.getEngineParam()->getIndex();
    engine = juce::jlimit(0, 15, engine);

    int paramIndex = 0;
    if (cfg.type == RowType::Harmonics) paramIndex = 0;
    else if (cfg.type == RowType::Timbre) paramIndex = 1;
    else if (cfg.type == RowType::Morph) paramIndex = 2;

    return kEngineParamLabels[engine][paramIndex];
}

void PlaitsVSTEditor::adjustValue(int row, int delta)
{
    int current = getDisplayValue(row);
    setDisplayValue(row, current + delta);
}

int PlaitsVSTEditor::rowAtY(int y) const
{
    int rowY = kTitleHeight;
    for (int i = 0; i < kNumRows; ++i) {
        if (y >= rowY && y < rowY + kRowHeight) {
            return i;
        }
        rowY += kRowHeight;
    }
    return -1;
}

void PlaitsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColor);

    // Title
    g.setColour(kTitleColor);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));
    g.drawText("PLAITS VST", 0, 4, getWidth(), kTitleHeight - 4, juce::Justification::centred);

    // Rows
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));

    int y = kTitleHeight;
    for (int i = 0; i < kNumRows; ++i) {
        const auto& cfg = kRowConfigs[i];
        bool selected = (i == selectedRow_);

        // Row background
        juce::Rectangle<int> rowRect(kPadding, y + kRowMargin,
                                      getWidth() - 2 * kPadding, kRowHeight - 2 * kRowMargin);
        g.setColour(kRowBgColor);
        g.fillRect(rowRect);

        // Value bar
        float norm = normalizedValue(i);
        int barWidth = static_cast<int>(norm * rowRect.getWidth());
        g.setColour(selected ? kBarSelectedColor : kBarColor);
        g.fillRect(rowRect.getX(), rowRect.getY(), barWidth, rowRect.getHeight());

        // Label (dynamic for Harmonics/Timbre/Morph based on engine)
        g.setColour(selected ? kTextSelectedColor : kTextColor);
        const char* label = getDynamicLabel(i);
        g.drawText(label, rowRect.getX() + 4, rowRect.getY(),
                   kLabelWidth, rowRect.getHeight(), juce::Justification::centredLeft);

        // Value
        juce::String valueStr = formatValue(i);
        g.drawText(valueStr, rowRect.getRight() - kValueWidth - 4, rowRect.getY(),
                   kValueWidth, rowRect.getHeight(), juce::Justification::centredRight);

        y += kRowHeight;
    }
}

void PlaitsVSTEditor::resized()
{
    // Fixed size, nothing to do
}

bool PlaitsVSTEditor::keyPressed(const juce::KeyPress& key)
{
    bool shift = key.getModifiers().isShiftDown();

    // Shift+S to save preset
    if (shift && key.getKeyCode() == 'S') {
        processor_.getPresetManager().saveCurrentAsNewPreset();
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::upKey) {
        selectedRow_ = (selectedRow_ - 1 + kNumRows) % kNumRows;
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::downKey) {
        selectedRow_ = (selectedRow_ + 1) % kNumRows;
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::leftKey) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int step = shift ? cfg.largeStep : cfg.smallStep;
        adjustValue(selectedRow_, -step);
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::rightKey) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int step = shift ? cfg.largeStep : cfg.smallStep;
        adjustValue(selectedRow_, step);
        return true;
    }

    return false;
}

void PlaitsVSTEditor::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();

    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;
        dragStartValue_ = getDisplayValue(row);
        dragStartX_ = event.x;
        repaint();
    }
}

void PlaitsVSTEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (selectedRow_ >= 0) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int deltaX = event.x - dragStartX_;

        if (cfg.type == RowType::Preset) {
            // For presets, use a wider drag threshold
            int numPresets = processor_.getPresetManager().getNumPresets();
            if (numPresets > 0) {
                float sensitivity = static_cast<float>(numPresets) / static_cast<float>(getWidth());
                int deltaValue = static_cast<int>(deltaX * sensitivity);
                setDisplayValue(selectedRow_, dragStartValue_ + deltaValue);
            }
        } else {
            // Map drag distance to value change: full width = full range
            float sensitivity = static_cast<float>(cfg.maxVal - cfg.minVal) / static_cast<float>(getWidth());
            int deltaValue = static_cast<int>(deltaX * sensitivity);
            setDisplayValue(selectedRow_, dragStartValue_ + deltaValue);
        }
    }
}

void PlaitsVSTEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;
        const auto& cfg = kRowConfigs[row];
        int delta = wheel.deltaY > 0 ? cfg.smallStep : -cfg.smallStep;
        adjustValue(row, delta);
    }
}
