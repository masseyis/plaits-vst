#include <gtest/gtest.h>
#include "dsp/resampler.h"
#include <cmath>

class ResamplerTest : public ::testing::Test {
protected:
    Resampler resampler_;
};

TEST_F(ResamplerTest, InitializesCorrectly) {
    resampler_.Init(48000.0, 44100.0);
    // Should not crash
}

TEST_F(ResamplerTest, SameRatePassthrough) {
    resampler_.Init(48000.0, 48000.0);

    int16_t input[100];
    float output[100];

    // Create a simple ramp
    for (int i = 0; i < 100; ++i) {
        input[i] = static_cast<int16_t>(i * 100);
    }

    size_t outputSize = resampler_.Process(input, 100, output, 100);

    // Same rate should produce approximately same number of samples
    EXPECT_GE(outputSize, 95);
    EXPECT_LE(outputSize, 105);
}

TEST_F(ResamplerTest, UpsampleProducesMoreSamples) {
    resampler_.Init(48000.0, 96000.0);  // Upsample 2x

    int16_t input[100];
    float output[300];

    for (int i = 0; i < 100; ++i) {
        input[i] = static_cast<int16_t>(i * 100);
    }

    size_t outputSize = resampler_.Process(input, 100, output, 300);

    // Should produce roughly 2x samples
    EXPECT_GE(outputSize, 180);
    EXPECT_LE(outputSize, 220);
}

TEST_F(ResamplerTest, DownsampleProducesFewerSamples) {
    resampler_.Init(48000.0, 24000.0);  // Downsample 2x

    int16_t input[100];
    float output[100];

    for (int i = 0; i < 100; ++i) {
        input[i] = static_cast<int16_t>(i * 100);
    }

    size_t outputSize = resampler_.Process(input, 100, output, 100);

    // Should produce roughly 0.5x samples
    EXPECT_GE(outputSize, 40);
    EXPECT_LE(outputSize, 60);
}

TEST_F(ResamplerTest, OutputInNormalizedRange) {
    resampler_.Init(48000.0, 44100.0);

    int16_t input[100];
    float output[100];

    // Full scale input
    for (int i = 0; i < 100; ++i) {
        input[i] = (i % 2 == 0) ? 32767 : -32768;
    }

    size_t outputSize = resampler_.Process(input, 100, output, 100);

    for (size_t i = 0; i < outputSize; ++i) {
        EXPECT_GE(output[i], -1.5f) << "Output should be approximately normalized";
        EXPECT_LE(output[i], 1.5f) << "Output should be approximately normalized";
    }
}

TEST_F(ResamplerTest, PreservesDCComponent) {
    resampler_.Init(48000.0, 44100.0);

    int16_t input[1000];
    float output[1000];

    // DC offset at half scale
    for (int i = 0; i < 1000; ++i) {
        input[i] = 16384;  // Half of max
    }

    size_t outputSize = resampler_.Process(input, 1000, output, 1000);

    // Calculate average of output (skip first few samples for filter settling)
    float sum = 0.0f;
    size_t start = std::min(outputSize / 4, static_cast<size_t>(100));
    for (size_t i = start; i < outputSize; ++i) {
        sum += output[i];
    }
    float avg = sum / static_cast<float>(outputSize - start);

    // Should preserve approximately 0.5 DC level
    EXPECT_NEAR(avg, 0.5f, 0.1f);
}

TEST_F(ResamplerTest, HandlesEmptyInput) {
    resampler_.Init(48000.0, 44100.0);

    float output[100];
    size_t outputSize = resampler_.Process(nullptr, 0, output, 100);

    EXPECT_EQ(outputSize, 0);
}

TEST_F(ResamplerTest, RespectsMaxOutputSize) {
    resampler_.Init(48000.0, 96000.0);  // 2x upsample

    int16_t input[100];
    float output[50];  // Smaller than expected output

    for (int i = 0; i < 100; ++i) {
        input[i] = static_cast<int16_t>(i * 100);
    }

    size_t outputSize = resampler_.Process(input, 100, output, 50);

    EXPECT_LE(outputSize, 50) << "Should not exceed max output size";
}

TEST_F(ResamplerTest, ConsecutiveCallsWork) {
    resampler_.Init(48000.0, 44100.0);

    int16_t input[100];
    float output[100];

    for (int i = 0; i < 100; ++i) {
        input[i] = static_cast<int16_t>(std::sin(i * 0.1) * 10000);
    }

    // Multiple consecutive calls
    for (int call = 0; call < 10; ++call) {
        size_t outputSize = resampler_.Process(input, 100, output, 100);
        EXPECT_GT(outputSize, 0) << "Call " << call << " should produce output";
    }
}
