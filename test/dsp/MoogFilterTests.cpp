// MoogFilter Tests
// Part of PlaitsVST - MIT License

#include <gtest/gtest.h>
#include "dsp/moog_filter.h"
#include <cmath>

using namespace plaits;

class MoogFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        filter_.Init(48000.0f);
    }

    MoogFilter filter_;
    static constexpr float kSampleRate = 48000.0f;
};

TEST_F(MoogFilterTest, InitDoesNotCrash) {
    // Verify default state
    EXPECT_FLOAT_EQ(filter_.GetCutoff(), 10000.0f);
    EXPECT_FLOAT_EQ(filter_.GetResonance(), 0.0f);
}

TEST_F(MoogFilterTest, SetCutoffWorks) {
    filter_.SetCutoff(5000.0f);
    EXPECT_FLOAT_EQ(filter_.GetCutoff(), 5000.0f);

    filter_.SetCutoff(1000.0f);
    EXPECT_FLOAT_EQ(filter_.GetCutoff(), 1000.0f);
}

TEST_F(MoogFilterTest, SetResonanceWorks) {
    filter_.SetResonance(0.5f);
    EXPECT_FLOAT_EQ(filter_.GetResonance(), 0.5f);

    filter_.SetResonance(1.0f);
    EXPECT_FLOAT_EQ(filter_.GetResonance(), 1.0f);
}

TEST_F(MoogFilterTest, CutoffClampsToValidRange) {
    filter_.SetCutoff(10.0f);  // Below minimum
    EXPECT_FLOAT_EQ(filter_.GetCutoff(), 20.0f);

    filter_.SetCutoff(25000.0f);  // Above maximum
    EXPECT_FLOAT_EQ(filter_.GetCutoff(), 20000.0f);
}

TEST_F(MoogFilterTest, ResonanceClampsToValidRange) {
    filter_.SetResonance(-0.5f);  // Below minimum
    EXPECT_FLOAT_EQ(filter_.GetResonance(), 0.0f);

    filter_.SetResonance(1.5f);  // Above maximum
    EXPECT_FLOAT_EQ(filter_.GetResonance(), 1.0f);
}

TEST_F(MoogFilterTest, ProcessSilenceReturnsSilence) {
    filter_.SetCutoff(1000.0f);
    filter_.SetResonance(0.5f);

    // Process silence
    for (int i = 0; i < 1000; ++i) {
        float output = filter_.Process(0.0f);
        EXPECT_NEAR(output, 0.0f, 0.001f);
    }
}

TEST_F(MoogFilterTest, LowpassFiltersHighFrequencies) {
    filter_.SetCutoff(100.0f);  // Very low cutoff
    filter_.SetResonance(0.0f);

    // Generate a high-frequency signal (4800 Hz at 48kHz = 10 samples per cycle)
    float sumInput = 0.0f;
    float sumOutput = 0.0f;

    for (int i = 0; i < 1000; ++i) {
        float input = std::sin(2.0f * 3.14159f * 4800.0f * i / kSampleRate);
        float output = filter_.Process(input);
        sumInput += std::abs(input);
        sumOutput += std::abs(output);
    }

    // Output should be significantly attenuated
    EXPECT_LT(sumOutput, sumInput * 0.1f);
}

TEST_F(MoogFilterTest, HighCutoffPassesSignal) {
    filter_.SetCutoff(20000.0f);  // Maximum cutoff
    filter_.SetResonance(0.0f);

    // Process a low-frequency signal
    float sumInput = 0.0f;
    float sumOutput = 0.0f;

    for (int i = 0; i < 1000; ++i) {
        float input = std::sin(2.0f * 3.14159f * 100.0f * i / kSampleRate);
        float output = filter_.Process(input);
        sumInput += std::abs(input);
        sumOutput += std::abs(output);
    }

    // Output should be close to input (allowing for some phase delay settling)
    EXPECT_GT(sumOutput, sumInput * 0.5f);
}

TEST_F(MoogFilterTest, ResonanceBoostsAtCutoff) {
    // First run with no resonance
    filter_.SetCutoff(1000.0f);
    filter_.SetResonance(0.0f);
    filter_.Reset();

    float sumNoRes = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        float input = std::sin(2.0f * 3.14159f * 1000.0f * i / kSampleRate);
        float output = filter_.Process(input);
        if (i > 500) {  // Skip initial transient
            sumNoRes += std::abs(output);
        }
    }

    // Now with resonance
    filter_.SetResonance(0.8f);
    filter_.Reset();

    float sumWithRes = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        float input = std::sin(2.0f * 3.14159f * 1000.0f * i / kSampleRate);
        float output = filter_.Process(input);
        if (i > 500) {
            sumWithRes += std::abs(output);
        }
    }

    // Resonance should boost the signal at cutoff frequency
    EXPECT_GT(sumWithRes, sumNoRes);
}

TEST_F(MoogFilterTest, ResetClearsState) {
    // Process some signal
    for (int i = 0; i < 100; ++i) {
        filter_.Process(1.0f);
    }

    // Reset
    filter_.Reset();

    // After reset, processing silence should give silence immediately
    float output = filter_.Process(0.0f);
    EXPECT_FLOAT_EQ(output, 0.0f);
}

TEST_F(MoogFilterTest, OutputStaysInReasonableRange) {
    filter_.SetCutoff(1000.0f);
    filter_.SetResonance(1.0f);  // Maximum resonance

    // Process a signal and ensure output doesn't explode
    for (int i = 0; i < 10000; ++i) {
        float input = std::sin(2.0f * 3.14159f * 500.0f * i / kSampleRate);
        float output = filter_.Process(input);

        // Output should stay within reasonable bounds even with high resonance
        EXPECT_GT(output, -10.0f);
        EXPECT_LT(output, 10.0f);
    }
}

TEST_F(MoogFilterTest, DifferentSampleRatesWork) {
    // Test at 44100 Hz
    MoogFilter filter44;
    filter44.Init(44100.0f);
    filter44.SetCutoff(1000.0f);

    float output44 = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output44 = filter44.Process(1.0f);
    }

    // Test at 96000 Hz
    MoogFilter filter96;
    filter96.Init(96000.0f);
    filter96.SetCutoff(1000.0f);

    float output96 = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output96 = filter96.Process(1.0f);
    }

    // Both should produce valid output
    EXPECT_GT(std::abs(output44), 0.0f);
    EXPECT_GT(std::abs(output96), 0.0f);
}
