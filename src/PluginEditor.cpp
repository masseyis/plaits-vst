#include "PluginEditor.h"
#include "PresetManager.h"
#include "dsp/modulation_matrix.h"
#include "dsp/lfo.h"

// Dynamic parameter labels per engine [engine][0=harmonics, 1=timbre, 2=morph]
const char* const PlaitsVSTEditor::kEngineParamLabels[16][3] = {
    {"DETUNE", "SQUARE", "SAW"},           // VA
    {"WAVEFORM", "FOLD", "ASYMMETRY"},     // Waveshaper
    {"RATIO", "MOD IDX", "FEEDBACK"},      // FM
    {"FORMNT 2", "FORMANT", "WIDTH"},      // Grain
    {"BUMPS", "HARMONIC", "SHAPE"},        // Additive
    {"BANK", "ROW", "COLUMN"},             // Wavetable
    {"CHORD", "INVERS", "WAVEFORM"},       // Chord
    {"TYPE", "SPECIES", "PHONEME"},        // Speech
    {"PITCH", "DENSITY", "OVERLAP"},       // Swarm
    {"FILTER", "CLOCK", "RESONAN"},        // Noise
    {"FREQ RND", "DENSITY", "REVERB"},     // Particle
    {"INHARM", "EXCITER", "DECAY"},        // String
    {"MATERIAL", "EXCITER", "DECAY"},      // Modal
    {"ATTACK", "TONE", "DECAY"},           // Bass Drum
    {"NOISE", "MODES", "DECAY"},           // Snare
    {"METAL", "HIGHPASS", "DECAY"},        // Hi-Hat
};

// Row configuration: label, type, min, max, smallStep, largeStep, suffix
const PlaitsVSTEditor::RowConfig PlaitsVSTEditor::kRowConfigs[kNumRows] = {
    {"PRESET",    RowType::Preset,     0,   0,    1,   1,  ""},
    {"ENGINE",    RowType::Engine,     0,   15,   1,   1,  ""},
    {"HARMONICS", RowType::Harmonics,  0,   127,  1,   13, ""},
    {"TIMBRE",    RowType::Timbre,     0,   127,  1,   13, ""},
    {"MORPH",     RowType::Morph,      0,   127,  1,   13, ""},
    {"ATTACK",    RowType::Attack,     0,   500,  5,   50, "ms"},
    {"DECAY",     RowType::Decay,      10,  2000, 20,  200,"ms"},
    {"VOICES",    RowType::Voices,     1,   16,   1,   2,  ""},
    {"CUTOFF",    RowType::Cutoff,     0,   127,  1,   13, ""},
    {"RESO",      RowType::Resonance,  0,   127,  1,   13, ""},
    {"LFO1",      RowType::Lfo1,       0,   0,    1,   1,  ""},  // Multi-field
    {"LFO2",      RowType::Lfo2,       0,   0,    1,   1,  ""},  // Multi-field
    {"ENV1",      RowType::Env1,       0,   0,    1,   1,  ""},  // Multi-field
    {"ENV2",      RowType::Env2,       0,   0,    1,   1,  ""},  // Multi-field
};

namespace {
    // Colors (same teal theme as BraidsVST)
    const juce::Colour kBgColor(0xff1a1a1a);
    const juce::Colour kRowBgColor(0xff0d0d0d);
    const juce::Colour kBarColor(0xff4a9090);
    const juce::Colour kBarSelectedColor(0xff6abfbf);
    const juce::Colour kModOverlayColor(0x80ffffff);
    const juce::Colour kTextColor(0xffa0a0a0);
    const juce::Colour kTextSelectedColor(0xffffffff);
    const juce::Colour kTextDimColor(0xff606060);
    const juce::Colour kTitleColor(0xffffffff);
    const juce::Colour kFieldSelectedColor(0xff8ad0d0);

    // Layout
    constexpr int kWindowWidth = 320;
    constexpr int kWindowHeight = 430;
    constexpr int kTitleHeight = 32;
    constexpr int kRowHeight = 26;
    constexpr int kRowMargin = 2;
    constexpr int kLabelWidth = 50;
    constexpr int kPadding = 8;
    constexpr int kModFieldGap = 4;

    // LFO rate names
    const char* kLfoRateNames[] = {"1/16", "1/8", "1/4", "1/2", "1BAR", "2BAR", "4BAR"};
    constexpr int kNumLfoRates = 7;

    // LFO shape names
    const char* kLfoShapeNames[] = {"TRI", "SAW", "SQR", "S&H"};
    constexpr int kNumLfoShapes = 4;
}

PlaitsVSTEditor::PlaitsVSTEditor(PlaitsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(kWindowWidth, kWindowHeight);
    setWantsKeyboardFocus(true);
    startTimerHz(30);

    // Grab keyboard focus after a short delay to ensure window is fully ready
    juce::Timer::callAfterDelay(100, [this]() {
        if (isShowing())
            grabKeyboardFocus();
    });
}

void PlaitsVSTEditor::parentHierarchyChanged()
{
    // Grab keyboard focus when the plugin window becomes part of the hierarchy
    if (getParentComponent() != nullptr)
        grabKeyboardFocus();
}

PlaitsVSTEditor::~PlaitsVSTEditor()
{
    stopTimer();
}

void PlaitsVSTEditor::timerCallback()
{
    repaint();
}

bool PlaitsVSTEditor::isModRow(int row) const
{
    const auto& cfg = kRowConfigs[row];
    return cfg.type == RowType::Lfo1 || cfg.type == RowType::Lfo2 ||
           cfg.type == RowType::Env1 || cfg.type == RowType::Env2;
}

int PlaitsVSTEditor::getNumFieldsForRow(int row) const
{
    return isModRow(row) ? 4 : 1;
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
        case RowType::Cutoff:
            return static_cast<int>(processor_.getCutoffParam()->get() * 127.0f + 0.5f);
        case RowType::Resonance:
            return static_cast<int>(processor_.getResonanceParam()->get() * 127.0f + 0.5f);
        // Mod rows handled by getModFieldValue
        default:
            return 0;
    }
}

void PlaitsVSTEditor::setDisplayValue(int row, int value)
{
    const auto& cfg = kRowConfigs[row];

    // Dynamic bounds for preset row
    int minVal = cfg.minVal;
    int maxVal = cfg.maxVal;
    if (cfg.type == RowType::Preset) {
        maxVal = juce::jmax(0, processor_.getPresetManager().getNumPresets() - 1);
    }
    value = juce::jlimit(minVal, maxVal, value);

    switch (cfg.type) {
        case RowType::Preset:
            processor_.getPresetManager().loadPreset(value);
            break;
        case RowType::Engine:
            processor_.getEngineParam()->setValueNotifyingHost(value / 15.0f);
            break;
        case RowType::Harmonics:
            processor_.getHarmonicsParam()->setValueNotifyingHost(value / 127.0f);
            break;
        case RowType::Timbre:
            processor_.getTimbreParam()->setValueNotifyingHost(value / 127.0f);
            break;
        case RowType::Morph:
            processor_.getMorphParam()->setValueNotifyingHost(value / 127.0f);
            break;
        case RowType::Attack:
            processor_.getAttackParam()->setValueNotifyingHost(value / 500.0f);
            break;
        case RowType::Decay:
            processor_.getDecayParam()->setValueNotifyingHost((value - 10.0f) / 1990.0f);
            break;
        case RowType::Voices:
            processor_.getPolyphonyParam()->setValueNotifyingHost((value - 1) / 15.0f);
            break;
        case RowType::Cutoff:
            processor_.getCutoffParam()->setValueNotifyingHost(value / 127.0f);
            break;
        case RowType::Resonance:
            processor_.getResonanceParam()->setValueNotifyingHost(value / 127.0f);
            break;
        default:
            break;
    }
}

int PlaitsVSTEditor::getModFieldValue(int row, int field) const
{
    const auto& cfg = kRowConfigs[row];

    // Convert actual dest to UI index for LFOs (skipping self-references)
    // LFO1: actual 7,8 -> UI 5,6 (skip actual 5,6)
    // LFO2: actual 0-6 -> UI 0-6 (skip actual 7,8)
    auto destToUiLfo1 = [](int dest) {
        if (dest >= 7) return dest - 2;  // actual 7,8 -> UI 5,6
        if (dest >= 5) return 4;  // actual 5,6 (invalid) -> clamp to UI 4
        return dest;
    };
    auto destToUiLfo2 = [](int dest) {
        if (dest >= 7) return 6;  // actual 7,8 (invalid) -> clamp to UI 6
        return dest;
    };

    if (cfg.type == RowType::Lfo1) {
        switch (field) {
            case 0: return processor_.getLfo1RateParam()->getIndex();
            case 1: return processor_.getLfo1ShapeParam()->getIndex();
            case 2: return destToUiLfo1(processor_.getLfo1DestParam()->getIndex());
            case 3: return std::abs(processor_.getLfo1AmountParam()->get());  // 0-63 for LFOs
        }
    } else if (cfg.type == RowType::Lfo2) {
        switch (field) {
            case 0: return processor_.getLfo2RateParam()->getIndex();
            case 1: return processor_.getLfo2ShapeParam()->getIndex();
            case 2: return destToUiLfo2(processor_.getLfo2DestParam()->getIndex());
            case 3: return std::abs(processor_.getLfo2AmountParam()->get());  // 0-63 for LFOs
        }
    } else if (cfg.type == RowType::Env1) {
        switch (field) {
            case 0: return static_cast<int>(processor_.getEnv1AttackParam()->get() * 500.0f + 0.5f);
            case 1: return static_cast<int>(processor_.getEnv1DecayParam()->get() * 1990.0f + 10.0f + 0.5f);
            case 2: return processor_.getEnv1DestParam()->getIndex();
            case 3: return processor_.getEnv1AmountParam()->get();
        }
    } else if (cfg.type == RowType::Env2) {
        switch (field) {
            case 0: return static_cast<int>(processor_.getEnv2AttackParam()->get() * 500.0f + 0.5f);
            case 1: return static_cast<int>(processor_.getEnv2DecayParam()->get() * 1990.0f + 10.0f + 0.5f);
            case 2: return processor_.getEnv2DestParam()->getIndex();
            case 3: return processor_.getEnv2AmountParam()->get();
        }
    }
    return 0;
}

void PlaitsVSTEditor::setModFieldValue(int row, int field, int value)
{
    const auto& cfg = kRowConfigs[row];

    // LFO1 valid destinations: 0-4, 7-8 (skip 5=LFO1RT, 6=LFO1AM)
    // LFO2 valid destinations: 0-6 (skip 7=LFO2RT, 8=LFO2AM)
    // UI index 0-6 maps to actual dest, skipping self-references
    auto uiToDestLfo1 = [](int ui) {
        ui = juce::jlimit(0, 6, ui);
        if (ui >= 5) ui += 2;  // UI 5,6 -> actual 7,8
        return ui;
    };
    auto uiToDestLfo2 = [](int ui) {
        return juce::jlimit(0, 6, ui);  // UI 0-6 -> actual 0-6
    };

    if (cfg.type == RowType::Lfo1) {
        switch (field) {
            case 0:  // Rate
                value = juce::jlimit(0, kNumLfoRates - 1, value);
                processor_.getLfo1RateParam()->setValueNotifyingHost(value / static_cast<float>(kNumLfoRates - 1));
                break;
            case 1:  // Shape
                value = juce::jlimit(0, kNumLfoShapes - 1, value);
                processor_.getLfo1ShapeParam()->setValueNotifyingHost(value / static_cast<float>(kNumLfoShapes - 1));
                break;
            case 2:  // Dest (skip LFO1RT, LFO1AM)
                value = uiToDestLfo1(value);
                processor_.getLfo1DestParam()->setValueNotifyingHost(value / 8.0f);
                break;
            case 3:  // Amount (0-63 for LFOs, stored as positive value)
                value = juce::jlimit(0, 63, value);
                processor_.getLfo1AmountParam()->setValueNotifyingHost((value + 64) / 127.0f);
                break;
        }
    } else if (cfg.type == RowType::Lfo2) {
        switch (field) {
            case 0:
                value = juce::jlimit(0, kNumLfoRates - 1, value);
                processor_.getLfo2RateParam()->setValueNotifyingHost(value / static_cast<float>(kNumLfoRates - 1));
                break;
            case 1:
                value = juce::jlimit(0, kNumLfoShapes - 1, value);
                processor_.getLfo2ShapeParam()->setValueNotifyingHost(value / static_cast<float>(kNumLfoShapes - 1));
                break;
            case 2:  // Dest (skip LFO2RT, LFO2AM)
                value = uiToDestLfo2(value);
                processor_.getLfo2DestParam()->setValueNotifyingHost(value / 8.0f);
                break;
            case 3:  // Amount (0-63 for LFOs, stored as positive value)
                value = juce::jlimit(0, 63, value);
                processor_.getLfo2AmountParam()->setValueNotifyingHost((value + 64) / 127.0f);
                break;
        }
    } else if (cfg.type == RowType::Env1) {
        switch (field) {
            case 0:  // Attack
                value = juce::jlimit(0, 500, value);
                processor_.getEnv1AttackParam()->setValueNotifyingHost(value / 500.0f);
                break;
            case 1:  // Decay
                value = juce::jlimit(10, 2000, value);
                processor_.getEnv1DecayParam()->setValueNotifyingHost((value - 10.0f) / 1990.0f);
                break;
            case 2:  // Dest
                value = juce::jlimit(0, 8, value);
                processor_.getEnv1DestParam()->setValueNotifyingHost(value / 8.0f);
                break;
            case 3:  // Amount
                value = juce::jlimit(-64, 63, value);
                processor_.getEnv1AmountParam()->setValueNotifyingHost((value + 64) / 127.0f);
                break;
        }
    } else if (cfg.type == RowType::Env2) {
        switch (field) {
            case 0:
                value = juce::jlimit(0, 500, value);
                processor_.getEnv2AttackParam()->setValueNotifyingHost(value / 500.0f);
                break;
            case 1:
                value = juce::jlimit(10, 2000, value);
                processor_.getEnv2DecayParam()->setValueNotifyingHost((value - 10.0f) / 1990.0f);
                break;
            case 2:
                value = juce::jlimit(0, 8, value);
                processor_.getEnv2DestParam()->setValueNotifyingHost(value / 8.0f);
                break;
            case 3:
                value = juce::jlimit(-64, 63, value);
                processor_.getEnv2AmountParam()->setValueNotifyingHost((value + 64) / 127.0f);
                break;
        }
    }
}

juce::String PlaitsVSTEditor::formatModFieldValue(int row, int field) const
{
    const auto& cfg = kRowConfigs[row];
    bool isLfo1 = (cfg.type == RowType::Lfo1);
    bool isLfo2 = (cfg.type == RowType::Lfo2);
    bool isLfo = isLfo1 || isLfo2;

    // Convert UI index back to actual dest for display
    auto uiToDestLfo1 = [](int ui) {
        if (ui >= 5) return ui + 2;  // UI 5,6 -> actual 7,8
        return ui;
    };
    auto uiToDestLfo2 = [](int ui) {
        return ui;  // UI 0-6 -> actual 0-6
    };

    if (isLfo) {
        switch (field) {
            case 0: {  // Rate
                int idx = getModFieldValue(row, field);
                return juce::String(kLfoRateNames[juce::jlimit(0, kNumLfoRates - 1, idx)]);
            }
            case 1: {  // Shape
                int idx = getModFieldValue(row, field);
                return juce::String(kLfoShapeNames[juce::jlimit(0, kNumLfoShapes - 1, idx)]);
            }
            case 2: {  // Dest - convert UI index to actual dest for display
                int uiIdx = getModFieldValue(row, field);
                int actualDest = isLfo1 ? uiToDestLfo1(uiIdx) : uiToDestLfo2(uiIdx);
                return getModDestinationName(actualDest);
            }
            case 3: {  // Amount - LFOs just show magnitude (0-63)
                int amt = getModFieldValue(row, field);
                return juce::String(std::abs(amt));
            }
        }
    } else {
        // ENV rows: Attack, Decay, Dest, Amount
        switch (field) {
            case 0: {  // Attack (ms)
                int ms = getModFieldValue(row, field);
                return juce::String(ms) + "ms";
            }
            case 1: {  // Decay (ms)
                int ms = getModFieldValue(row, field);
                return juce::String(ms) + "ms";
            }
            case 2: {  // Dest
                int idx = getModFieldValue(row, field);
                return getModDestinationName(idx);
            }
            case 3: {  // Amount
                int amt = getModFieldValue(row, field);
                return (amt >= 0 ? "+" : "") + juce::String(amt);
            }
        }
    }
    return "";
}

int PlaitsVSTEditor::getLargeStepForModField(int row, int field) const
{
    const auto& cfg = kRowConfigs[row];
    bool isLfo = (cfg.type == RowType::Lfo1 || cfg.type == RowType::Lfo2);

    if (isLfo) {
        switch (field) {
            case 0: return 1;   // Rate - cycle through all
            case 1: return 1;   // Shape - cycle through all
            case 2: return 1;   // Dest - cycle through all
            case 3: return 8;   // Amount - 10% steps
        }
    } else {
        // ENV rows
        switch (field) {
            case 0: return 50;   // Attack (ms) - 50ms steps
            case 1: return 200;  // Decay (ms) - 200ms steps
            case 2: return 1;    // Dest - cycle through all
            case 3: return 8;    // Amount - 10% steps
        }
    }
    return 1;
}

juce::String PlaitsVSTEditor::getModDestinationName(int destIndex) const
{
    int engine = processor_.getEngineParam()->getIndex();
    switch (destIndex) {
        case 0: return juce::String(kEngineParamLabels[engine][0]).substring(0, 7);
        case 1: return juce::String(kEngineParamLabels[engine][1]).substring(0, 7);
        case 2: return juce::String(kEngineParamLabels[engine][2]).substring(0, 7);
        case 3: return "CUTOFF";
        case 4: return "RESO";
        case 5: return "LFO1RT";
        case 6: return "LFO1AM";
        case 7: return "LFO2RT";
        case 8: return "LFO2AM";
        default: return "???";
    }
}

const char* PlaitsVSTEditor::getDynamicLabel(int row) const
{
    int engine = processor_.getEngineParam()->getIndex();
    const auto& cfg = kRowConfigs[row];

    switch (cfg.type) {
        case RowType::Harmonics:
            return kEngineParamLabels[engine][0];
        case RowType::Timbre:
            return kEngineParamLabels[engine][1];
        case RowType::Morph:
            return kEngineParamLabels[engine][2];
        default:
            return cfg.label;
    }
}

juce::String PlaitsVSTEditor::formatValue(int row) const
{
    const auto& cfg = kRowConfigs[row];
    int value = getDisplayValue(row);

    switch (cfg.type) {
        case RowType::Preset: {
            auto& pm = processor_.getPresetManager();
            juce::String name = pm.getCurrentPresetName();
            if (pm.isModified()) name += " *";
            return name;
        }
        case RowType::Engine:
            return engineNames_[value];
        default:
            return juce::String(value) + cfg.suffix;
    }
}

float PlaitsVSTEditor::normalizedValue(int row) const
{
    const auto& cfg = kRowConfigs[row];
    if (cfg.maxVal == cfg.minVal) return 0.0f;
    int value = getDisplayValue(row);
    return static_cast<float>(value - cfg.minVal) / (cfg.maxVal - cfg.minVal);
}

void PlaitsVSTEditor::adjustValue(int row, int delta)
{
    if (isModRow(row)) {
        int current = getModFieldValue(row, selectedField_);
        setModFieldValue(row, selectedField_, current + delta);
    } else {
        setDisplayValue(row, getDisplayValue(row) + delta);
    }
}

bool PlaitsVSTEditor::hasModOverlay(int row) const
{
    const auto& cfg = kRowConfigs[row];
    return cfg.type == RowType::Harmonics || cfg.type == RowType::Timbre ||
           cfg.type == RowType::Morph || cfg.type == RowType::Cutoff ||
           cfg.type == RowType::Resonance;
}

float PlaitsVSTEditor::getModOverlayValue(int row) const
{
    const auto& cfg = kRowConfigs[row];
    switch (cfg.type) {
        case RowType::Harmonics: return processor_.getModulatedHarmonics();
        case RowType::Timbre: return processor_.getModulatedTimbre();
        case RowType::Morph: return processor_.getModulatedMorph();
        case RowType::Cutoff: return processor_.getModulatedCutoff();
        case RowType::Resonance: return processor_.getModulatedResonance();
        default: return 0.0f;
    }
}

int PlaitsVSTEditor::rowAtY(int y) const
{
    if (y < kTitleHeight) return -1;
    int row = (y - kTitleHeight) / (kRowHeight + kRowMargin);
    return juce::jlimit(0, kNumRows - 1, row);
}

void PlaitsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColor);

    // Title bar
    g.setColour(kTitleColor);
    auto font = juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold);
    g.setFont(font);
    g.drawText("PlaitsVST", 0, 0, getWidth(), kTitleHeight, juce::Justification::centred);

    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));

    // Draw rows
    for (int row = 0; row < kNumRows; ++row) {
        const auto& cfg = kRowConfigs[row];
        bool selected = (row == selectedRow_);

        int y = kTitleHeight + row * (kRowHeight + kRowMargin);
        juce::Rectangle<int> rowRect(kPadding, y, getWidth() - kPadding * 2, kRowHeight);

        // Row background
        g.setColour(kRowBgColor);
        g.fillRoundedRectangle(rowRect.toFloat(), 3.0f);

        if (isModRow(row)) {
            paintModRow(g, row, rowRect, selected);
        } else {
            // Standard row
            const char* label = getDynamicLabel(row);
            g.setColour(selected ? kTextSelectedColor : kTextColor);
            g.drawText(label, rowRect.getX() + 4, rowRect.getY(),
                       kLabelWidth - 8, rowRect.getHeight(), juce::Justification::centredLeft);

            // Value bar area
            int barX = rowRect.getX() + kLabelWidth;
            int barWidth = rowRect.getWidth() - kLabelWidth - 4;
            juce::Rectangle<int> barRect(barX, rowRect.getY() + 4, barWidth, rowRect.getHeight() - 8);

            // Draw bar
            float norm = normalizedValue(row);
            g.setColour(selected ? kBarSelectedColor : kBarColor);
            g.fillRoundedRectangle(barRect.getX(), barRect.getY(),
                                   barRect.getWidth() * norm, barRect.getHeight(), 2.0f);

            // Draw modulation overlay if applicable
            if (hasModOverlay(row)) {
                paintModulationOverlay(g, row, barRect);
            }

            // Draw value text
            g.setColour(selected ? kTextSelectedColor : kTextColor);
            g.drawText(formatValue(row), barRect, juce::Justification::centred);
        }

        // Selection indicator
        if (selected) {
            g.setColour(kBarSelectedColor);
            g.drawRoundedRectangle(rowRect.toFloat(), 3.0f, 1.0f);
        }
    }
}

void PlaitsVSTEditor::paintModRow(juce::Graphics& g, int row, const juce::Rectangle<int>& rowRect, bool selected)
{
    const auto& cfg = kRowConfigs[row];
    int numFields = getNumFieldsForRow(row);

    // Draw row label
    g.setColour(selected ? kTextSelectedColor : kTextColor);
    g.drawText(cfg.label, rowRect.getX() + 4, rowRect.getY(),
               kLabelWidth - 8, rowRect.getHeight(), juce::Justification::centredLeft);

    // Calculate field positions
    int fieldX = rowRect.getX() + kLabelWidth;
    int availableWidth = rowRect.getWidth() - kLabelWidth - 4;
    int fieldWidth = (availableWidth - (numFields - 1) * kModFieldGap) / numFields;

    for (int f = 0; f < numFields; ++f) {
        juce::Rectangle<int> fieldRect(fieldX, rowRect.getY() + 2,
                                        fieldWidth, rowRect.getHeight() - 4);

        // Field background - highlight if selected
        bool isSelectedField = selected && (f == selectedField_);
        g.setColour(isSelectedField ? kFieldSelectedColor.withAlpha(0.3f) : kRowBgColor.brighter(0.1f));
        g.fillRoundedRectangle(fieldRect.toFloat(), 2.0f);

        // Field border if selected
        if (isSelectedField) {
            g.setColour(kFieldSelectedColor);
            g.drawRoundedRectangle(fieldRect.toFloat(), 2.0f, 1.0f);
        }

        // Field value
        g.setColour(isSelectedField ? kTextSelectedColor : kTextColor);
        juce::String valueStr = formatModFieldValue(row, f);
        g.drawText(valueStr, fieldRect, juce::Justification::centred);

        fieldX += fieldWidth + kModFieldGap;
    }
}

void PlaitsVSTEditor::paintModulationOverlay(juce::Graphics& g, int row, const juce::Rectangle<int>& barRect)
{
    float baseValue = normalizedValue(row);
    float modulatedValue = getModOverlayValue(row);

    float diff = modulatedValue - baseValue;
    if (std::abs(diff) < 0.001f) return;

    int baseX = barRect.getX() + static_cast<int>(baseValue * barRect.getWidth());
    int modX = barRect.getX() + static_cast<int>(modulatedValue * barRect.getWidth());

    int leftX = std::min(baseX, modX);
    int rightX = std::max(baseX, modX);

    g.setColour(kModOverlayColor);
    g.fillRect(leftX, barRect.getY(), rightX - leftX, barRect.getHeight());

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawVerticalLine(modX, static_cast<float>(barRect.getY()),
                       static_cast<float>(barRect.getBottom()));
}

void PlaitsVSTEditor::resized()
{
}

bool PlaitsVSTEditor::keyPressed(const juce::KeyPress& key)
{
    const auto& cfg = kRowConfigs[selectedRow_];
    int smallStep = cfg.smallStep;
    int largeStep = cfg.largeStep;
    int keyCode = key.getKeyCode();
    bool shiftDown = key.getModifiers().isShiftDown();

    if (keyCode == juce::KeyPress::upKey) {
        int newRow = juce::jmax(0, selectedRow_ - 1);
        // Maintain field position when switching between mod rows
        if (isModRow(newRow)) {
            selectedField_ = juce::jmin(selectedField_, getNumFieldsForRow(newRow) - 1);
        } else {
            selectedField_ = 0;
        }
        selectedRow_ = newRow;
        repaint();
        return true;
    }
    if (keyCode == juce::KeyPress::downKey) {
        int newRow = juce::jmin(kNumRows - 1, selectedRow_ + 1);
        // Maintain field position when switching between mod rows
        if (isModRow(newRow)) {
            selectedField_ = juce::jmin(selectedField_, getNumFieldsForRow(newRow) - 1);
        } else {
            selectedField_ = 0;
        }
        selectedRow_ = newRow;
        repaint();
        return true;
    }
    if (keyCode == juce::KeyPress::leftKey) {
        if (isModRow(selectedRow_)) {
            // For mod rows: use large step with shift for values
            int step = shiftDown ? getLargeStepForModField(selectedRow_, selectedField_) : smallStep;
            adjustValue(selectedRow_, -step);
        } else {
            adjustValue(selectedRow_, shiftDown ? -largeStep : -smallStep);
        }
        repaint();
        return true;
    }
    if (keyCode == juce::KeyPress::rightKey) {
        if (isModRow(selectedRow_)) {
            // For mod rows: use large step with shift for values
            int step = shiftDown ? getLargeStepForModField(selectedRow_, selectedField_) : smallStep;
            adjustValue(selectedRow_, step);
        } else {
            adjustValue(selectedRow_, shiftDown ? largeStep : smallStep);
        }
        repaint();
        return true;
    }

    // Tab/Shift+Tab to switch fields in mod rows
    if (keyCode == juce::KeyPress::tabKey) {
        if (isModRow(selectedRow_)) {
            int numFields = getNumFieldsForRow(selectedRow_);
            if (shiftDown) {
                selectedField_ = (selectedField_ - 1 + numFields) % numFields;
            } else {
                selectedField_ = (selectedField_ + 1) % numFields;
            }
            repaint();
            return true;
        }
    }

    // S key to save preset
    if (key.getTextCharacter() == 's' || key.getTextCharacter() == 'S') {
        processor_.getPresetManager().saveCurrentAsNewPreset();
        repaint();
        return true;
    }

    return false;
}

void PlaitsVSTEditor::mouseDown(const juce::MouseEvent& event)
{
    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;

        // For mod rows, determine which field was clicked
        if (isModRow(row)) {
            int y = kTitleHeight + row * (kRowHeight + kRowMargin);
            juce::Rectangle<int> rowRect(kPadding, y, getWidth() - kPadding * 2, kRowHeight);

            int fieldX = rowRect.getX() + kLabelWidth;
            int availableWidth = rowRect.getWidth() - kLabelWidth - 4;
            int numFields = getNumFieldsForRow(row);
            int fieldWidth = (availableWidth - (numFields - 1) * kModFieldGap) / numFields;

            for (int f = 0; f < numFields; ++f) {
                int left = fieldX + f * (fieldWidth + kModFieldGap);
                int right = left + fieldWidth;
                if (event.x >= left && event.x < right) {
                    selectedField_ = f;
                    break;
                }
            }
            dragStartValue_ = getModFieldValue(row, selectedField_);
        } else {
            selectedField_ = 0;
            dragStartValue_ = getDisplayValue(row);
        }
        dragStartX_ = event.x;
        repaint();
    }
}

void PlaitsVSTEditor::mouseDrag(const juce::MouseEvent& event)
{
    int deltaX = event.x - dragStartX_;
    int delta = deltaX / 3;

    if (isModRow(selectedRow_)) {
        setModFieldValue(selectedRow_, selectedField_, dragStartValue_ + delta);
    } else {
        setDisplayValue(selectedRow_, dragStartValue_ + delta);
    }
    repaint();
}

void PlaitsVSTEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;

        int delta = (wheel.deltaY > 0) ? 1 : -1;
        if (wheel.deltaY != 0) {
            adjustValue(row, delta);
        }
        repaint();
    }
}
