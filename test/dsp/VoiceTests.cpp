#include <gtest/gtest.h>
#include "dsp/voice.h"
#include <cmath>
#include <numeric>

class VoiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        voice_.Init(44100.0);
    }

    Voice voice_;
};

TEST_F(VoiceTest, InitialState) {
    EXPECT_FALSE(voice_.active());
    EXPECT_EQ(voice_.note(), -1);
}

TEST_F(VoiceTest, NoteOnActivatesVoice) {
    voice_.NoteOn(60, 1.0f, 10.0f, 100.0f);
    EXPECT_TRUE(voice_.active());
    EXPECT_EQ(voice_.note(), 60);
}

TEST_F(VoiceTest, NoteOffWithADEnvelope) {
    voice_.NoteOn(60, 1.0f, 10.0f, 100.0f);
    voice_.NoteOff();
    // AD envelope doesn't respond to note off - voice stays active until decay completes
    // Note is retained until voice becomes inactive
    EXPECT_TRUE(voice_.active());
    EXPECT_EQ(voice_.note(), 60);
}

TEST_F(VoiceTest, ProcessProducesOutput) {
    voice_.NoteOn(60, 1.0f, 1.0f, 100.0f);  // Short attack

    float left[256], right[256];
    std::fill(left, left + 256, 0.0f);
    std::fill(right, right + 256, 0.0f);

    voice_.Process(left, right, 256);

    // Should produce some non-zero output
    float sumLeft = 0.0f, sumRight = 0.0f;
    for (int i = 0; i < 256; ++i) {
        sumLeft += std::abs(left[i]);
        sumRight += std::abs(right[i]);
    }

    EXPECT_GT(sumLeft, 0.0f) << "Should produce left channel output";
    EXPECT_GT(sumRight, 0.0f) << "Should produce right channel output";
}

TEST_F(VoiceTest, InactiveVoiceDoesNotModifyBuffer) {
    float left[256], right[256];
    std::fill(left, left + 256, 1.0f);  // Fill with non-zero
    std::fill(right, right + 256, 1.0f);

    voice_.Process(left, right, 256);

    // Inactive voice should not modify the buffer (it adds to existing content)
    // Since voice is inactive, buffer should remain unchanged
    for (int i = 0; i < 256; ++i) {
        EXPECT_FLOAT_EQ(left[i], 1.0f) << "Inactive voice should not modify buffer";
        EXPECT_FLOAT_EQ(right[i], 1.0f) << "Inactive voice should not modify buffer";
    }
}

TEST_F(VoiceTest, DifferentNotesProduceDifferentPitches) {
    // Test with a low note
    voice_.Init(44100.0);
    voice_.set_engine(0);  // VA engine
    voice_.NoteOn(36, 1.0f, 0.0f, 500.0f);  // C2

    float lowLeft[4096], lowRight[4096];
    std::fill(lowLeft, lowLeft + 4096, 0.0f);
    std::fill(lowRight, lowRight + 4096, 0.0f);
    voice_.Process(lowLeft, lowRight, 4096);

    // Test with a high note
    Voice voice2;
    voice2.Init(44100.0);
    voice2.set_engine(0);
    voice2.NoteOn(72, 1.0f, 0.0f, 500.0f);  // C5

    float highLeft[4096], highRight[4096];
    std::fill(highLeft, highLeft + 4096, 0.0f);
    std::fill(highRight, highRight + 4096, 0.0f);
    voice2.Process(highLeft, highRight, 4096);

    // Count zero crossings as a rough frequency measure
    int lowCrossings = 0, highCrossings = 0;
    for (int i = 1; i < 4096; ++i) {
        if ((lowLeft[i-1] < 0 && lowLeft[i] >= 0) || (lowLeft[i-1] >= 0 && lowLeft[i] < 0))
            lowCrossings++;
        if ((highLeft[i-1] < 0 && highLeft[i] >= 0) || (highLeft[i-1] >= 0 && highLeft[i] < 0))
            highCrossings++;
    }

    EXPECT_GT(highCrossings, lowCrossings) << "Higher note should have more zero crossings";
}

TEST_F(VoiceTest, VelocityAffectsAmplitude) {
    // Loud note
    voice_.Init(44100.0);
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float loudLeft[256], loudRight[256];
    std::fill(loudLeft, loudLeft + 256, 0.0f);
    std::fill(loudRight, loudRight + 256, 0.0f);
    voice_.Process(loudLeft, loudRight, 256);

    float loudSum = 0.0f;
    for (int i = 0; i < 256; ++i) {
        loudSum += loudLeft[i] * loudLeft[i];
    }

    // Quiet note
    Voice quietVoice;
    quietVoice.Init(44100.0);
    quietVoice.NoteOn(60, 0.25f, 0.0f, 500.0f);

    float quietLeft[256], quietRight[256];
    std::fill(quietLeft, quietLeft + 256, 0.0f);
    std::fill(quietRight, quietRight + 256, 0.0f);
    quietVoice.Process(quietLeft, quietRight, 256);

    float quietSum = 0.0f;
    for (int i = 0; i < 256; ++i) {
        quietSum += quietLeft[i] * quietLeft[i];
    }

    EXPECT_GT(loudSum, quietSum) << "Higher velocity should produce louder output";
}

TEST_F(VoiceTest, EngineParameterChanges) {
    voice_.set_engine(0);  // VA
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left1[256], right1[256];
    std::fill(left1, left1 + 256, 0.0f);
    std::fill(right1, right1 + 256, 0.0f);
    voice_.Process(left1, right1, 256);

    // Change to different engine
    Voice voice2;
    voice2.Init(44100.0);
    voice2.set_engine(5);  // Wavetable
    voice2.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left2[256], right2[256];
    std::fill(left2, left2 + 256, 0.0f);
    std::fill(right2, right2 + 256, 0.0f);
    voice2.Process(left2, right2, 256);

    // Different engines should produce different waveforms
    float diff = 0.0f;
    for (int i = 0; i < 256; ++i) {
        diff += std::abs(left1[i] - left2[i]);
    }

    EXPECT_GT(diff, 0.1f) << "Different engines should produce different output";
}

TEST_F(VoiceTest, AllEnginesWork) {
    for (int engine = 0; engine < 16; ++engine) {
        Voice v;
        v.Init(44100.0);
        v.set_engine(engine);
        v.NoteOn(60, 1.0f, 0.0f, 500.0f);

        float left[512], right[512];
        std::fill(left, left + 512, 0.0f);
        std::fill(right, right + 512, 0.0f);

        // Should not crash
        v.Process(left, right, 512);

        float sum = 0.0f;
        for (int i = 0; i < 512; ++i) {
            sum += std::abs(left[i]);
        }

        // Most engines should produce some output (some may be quieter)
        // Just verify no crash and reasonable values
        for (int i = 0; i < 512; ++i) {
            EXPECT_FALSE(std::isnan(left[i])) << "Engine " << engine << " produced NaN";
            EXPECT_FALSE(std::isinf(left[i])) << "Engine " << engine << " produced Inf";
        }
    }
}

TEST_F(VoiceTest, HarmonicsParameterWorks) {
    voice_.set_harmonics(0.0f);
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float low[512], lowR[512];
    std::fill(low, low + 512, 0.0f);
    std::fill(lowR, lowR + 512, 0.0f);
    voice_.Process(low, lowR, 512);

    voice_.set_harmonics(1.0f);
    float high[512], highR[512];
    std::fill(high, high + 512, 0.0f);
    std::fill(highR, highR + 512, 0.0f);
    voice_.Process(high, highR, 512);

    // Different harmonics settings should produce different output
    float diff = 0.0f;
    for (int i = 0; i < 512; ++i) {
        diff += std::abs(low[i] - high[i]);
    }

    EXPECT_GT(diff, 0.01f) << "Different harmonics should produce different output";
}

TEST_F(VoiceTest, TimbreParameterWorks) {
    voice_.set_timbre(0.0f);
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float low[512], lowR[512];
    std::fill(low, low + 512, 0.0f);
    std::fill(lowR, lowR + 512, 0.0f);
    voice_.Process(low, lowR, 512);

    voice_.set_timbre(1.0f);
    float high[512], highR[512];
    std::fill(high, high + 512, 0.0f);
    std::fill(highR, highR + 512, 0.0f);
    voice_.Process(high, highR, 512);

    float diff = 0.0f;
    for (int i = 0; i < 512; ++i) {
        diff += std::abs(low[i] - high[i]);
    }

    EXPECT_GT(diff, 0.01f) << "Different timbre should produce different output";
}

TEST_F(VoiceTest, MorphParameterWorks) {
    voice_.set_morph(0.0f);
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float low[512], lowR[512];
    std::fill(low, low + 512, 0.0f);
    std::fill(lowR, lowR + 512, 0.0f);
    voice_.Process(low, lowR, 512);

    voice_.set_morph(1.0f);
    float high[512], highR[512];
    std::fill(high, high + 512, 0.0f);
    std::fill(highR, highR + 512, 0.0f);
    voice_.Process(high, highR, 512);

    float diff = 0.0f;
    for (int i = 0; i < 512; ++i) {
        diff += std::abs(low[i] - high[i]);
    }

    EXPECT_GT(diff, 0.01f) << "Different morph should produce different output";
}
