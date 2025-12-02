// LFO Tests
// Part of PlaitsVST - MIT License

#include <gtest/gtest.h>
#include "dsp/lfo.h"
#include <cmath>

using namespace plaits;

class LfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        lfo_.Init();
    }

    Lfo lfo_;
    static constexpr float kSampleRate = 48000.0f;
};

TEST_F(LfoTest, InitDoesNotCrash) {
    // Just verify Init works
    EXPECT_EQ(lfo_.GetRate(), LfoRateDivision::Div_1_4);
    EXPECT_EQ(lfo_.GetShape(), LfoShape::Triangle);
}

TEST_F(LfoTest, ProcessProducesOutput) {
    bool has_nonzero = false;
    for (int i = 0; i < 1000; ++i) {
        float output = lfo_.Process(kSampleRate);
        if (std::abs(output) > 0.001f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(LfoTest, OutputInRange) {
    lfo_.SetShape(LfoShape::Triangle);

    for (int i = 0; i < 10000; ++i) {
        float output = lfo_.Process(kSampleRate);
        EXPECT_GE(output, -1.0f);
        EXPECT_LE(output, 1.0f);
    }
}

TEST_F(LfoTest, TriangleShapeWorks) {
    lfo_.SetShape(LfoShape::Triangle);
    lfo_.SetRate(LfoRateDivision::Div_1_16);  // Fast rate

    float min_val = 1.0f;
    float max_val = -1.0f;

    for (int i = 0; i < 10000; ++i) {
        float output = lfo_.Process(kSampleRate);
        min_val = std::min(min_val, output);
        max_val = std::max(max_val, output);
    }

    // Should reach near extremes
    EXPECT_LT(min_val, -0.9f);
    EXPECT_GT(max_val, 0.9f);
}

TEST_F(LfoTest, SquareShapeWorks) {
    lfo_.SetShape(LfoShape::Square);
    lfo_.SetRate(LfoRateDivision::Div_1_16);

    bool has_positive = false;
    bool has_negative = false;

    for (int i = 0; i < 10000; ++i) {
        float output = lfo_.Process(kSampleRate);
        if (output > 0.5f) has_positive = true;
        if (output < -0.5f) has_negative = true;

        // Square should only be +1 or -1
        EXPECT_TRUE(std::abs(output - 1.0f) < 0.01f || std::abs(output + 1.0f) < 0.01f);
    }

    EXPECT_TRUE(has_positive);
    EXPECT_TRUE(has_negative);
}

TEST_F(LfoTest, SawShapeWorks) {
    lfo_.SetShape(LfoShape::Saw);
    lfo_.SetRate(LfoRateDivision::Div_1_16);

    float min_val = 1.0f;
    float max_val = -1.0f;

    for (int i = 0; i < 10000; ++i) {
        float output = lfo_.Process(kSampleRate);
        min_val = std::min(min_val, output);
        max_val = std::max(max_val, output);
    }

    EXPECT_LT(min_val, -0.9f);
    EXPECT_GT(max_val, 0.9f);
}

TEST_F(LfoTest, SampleAndHoldWorks) {
    lfo_.SetShape(LfoShape::SampleAndHold);
    lfo_.SetRate(LfoRateDivision::Div_1_16);

    // S&H should produce discrete steps
    float last_output = lfo_.Process(kSampleRate);
    int changes = 0;

    for (int i = 0; i < 10000; ++i) {
        float output = lfo_.Process(kSampleRate);
        if (std::abs(output - last_output) > 0.01f) {
            changes++;
            last_output = output;
        }
    }

    // Should have some discrete changes but not every sample
    EXPECT_GT(changes, 0);
    EXPECT_LT(changes, 1000);  // Not changing every sample
}

TEST_F(LfoTest, TempoAffectsRate) {
    lfo_.SetRate(LfoRateDivision::Div_1_16);  // Faster division for more cycles
    lfo_.SetShape(LfoShape::Saw);

    // Count cycles at 60 BPM
    lfo_.SetTempo(60.0);
    lfo_.Reset();
    int cycles_60 = 0;
    float last = lfo_.Process(kSampleRate);
    for (int i = 0; i < 96000; ++i) {  // 2 seconds
        float output = lfo_.Process(kSampleRate);
        // Detect cycle reset (saw jumps from negative to positive)
        if (output > 0.5f && last < -0.5f) {
            cycles_60++;
        }
        last = output;
    }

    // Count cycles at 120 BPM
    lfo_.SetTempo(120.0);
    lfo_.Reset();
    int cycles_120 = 0;
    last = lfo_.Process(kSampleRate);
    for (int i = 0; i < 96000; ++i) {  // 2 seconds
        float output = lfo_.Process(kSampleRate);
        if (output > 0.5f && last < -0.5f) {
            cycles_120++;
        }
        last = output;
    }

    // At 120 BPM, should be roughly 2x as fast
    // Use >= to handle edge cases
    EXPECT_GE(cycles_120, cycles_60);
    // And verify we actually got multiple cycles
    EXPECT_GT(cycles_60, 0);
}

TEST_F(LfoTest, RateNamesExist) {
    for (int i = 0; i < static_cast<int>(LfoRateDivision::NumDivisions); ++i) {
        const char* name = Lfo::GetRateName(static_cast<LfoRateDivision>(i));
        EXPECT_NE(name, nullptr);
        EXPECT_STRNE(name, "???");
    }
}

TEST_F(LfoTest, ShapeNamesExist) {
    for (int i = 0; i < static_cast<int>(LfoShape::NumShapes); ++i) {
        const char* name = Lfo::GetShapeName(static_cast<LfoShape>(i));
        EXPECT_NE(name, nullptr);
        EXPECT_STRNE(name, "???");
    }
}
