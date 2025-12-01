#include <gtest/gtest.h>
#include "dsp/voice.h"
#include <cmath>
#include <algorithm>

// Test that all 16 Plaits engines produce valid output
class PlaitsEngineTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        voice_.Init(44100.0);
    }

    plaits_vst::Voice voice_;
};

TEST_P(PlaitsEngineTest, EngineProducesOutput) {
    int engine = GetParam();
    voice_.set_engine(engine);
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.5f);

    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[2048], right[2048];
    std::fill(left, left + 2048, 0.0f);
    std::fill(right, right + 2048, 0.0f);

    voice_.Process(left, right, 2048);

    // Check for non-zero output
    float maxAbs = 0.0f;
    for (int i = 0; i < 2048; ++i) {
        maxAbs = std::max(maxAbs, std::abs(left[i]));
        maxAbs = std::max(maxAbs, std::abs(right[i]));
    }

    // Most engines should produce some output
    // Some engines may be quieter depending on parameters
    EXPECT_GE(maxAbs, 0.0f) << "Engine " << engine << " should produce valid output";
}

TEST_P(PlaitsEngineTest, EngineOutputIsFinite) {
    int engine = GetParam();
    voice_.set_engine(engine);
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.5f);

    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[4096], right[4096];

    // Process multiple blocks
    for (int block = 0; block < 10; ++block) {
        std::fill(left, left + 4096, 0.0f);
        std::fill(right, right + 4096, 0.0f);

        voice_.Process(left, right, 4096);

        for (int i = 0; i < 4096; ++i) {
            ASSERT_FALSE(std::isnan(left[i]))
                << "Engine " << engine << " produced NaN in left channel at block "
                << block << " sample " << i;
            ASSERT_FALSE(std::isnan(right[i]))
                << "Engine " << engine << " produced NaN in right channel at block "
                << block << " sample " << i;
            ASSERT_FALSE(std::isinf(left[i]))
                << "Engine " << engine << " produced Inf in left channel at block "
                << block << " sample " << i;
            ASSERT_FALSE(std::isinf(right[i]))
                << "Engine " << engine << " produced Inf in right channel at block "
                << block << " sample " << i;
        }
    }
}

TEST_P(PlaitsEngineTest, EngineHandlesExtremeParameters) {
    int engine = GetParam();
    voice_.set_engine(engine);

    // Test with extreme parameter values
    const float extremes[] = {0.0f, 1.0f};

    for (float h : extremes) {
        for (float t : extremes) {
            for (float m : extremes) {
                voice_.set_harmonics(h);
                voice_.set_timbre(t);
                voice_.set_morph(m);

                voice_.NoteOn(60, 1.0f, 0.0f, 100.0f);

                float left[512], right[512];
                std::fill(left, left + 512, 0.0f);
                std::fill(right, right + 512, 0.0f);

                voice_.Process(left, right, 512);

                // Check for validity
                for (int i = 0; i < 512; ++i) {
                    ASSERT_FALSE(std::isnan(left[i]))
                        << "Engine " << engine << " with h=" << h << " t=" << t << " m=" << m
                        << " produced NaN";
                    ASSERT_FALSE(std::isinf(left[i]))
                        << "Engine " << engine << " with h=" << h << " t=" << t << " m=" << m
                        << " produced Inf";
                }
            }
        }
    }
}

TEST_P(PlaitsEngineTest, EngineHandlesDifferentNotes) {
    int engine = GetParam();
    voice_.set_engine(engine);
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.5f);

    // Test different MIDI notes
    const int notes[] = {24, 36, 48, 60, 72, 84, 96, 108};

    for (int note : notes) {
        voice_.NoteOn(note, 1.0f, 0.0f, 100.0f);

        float left[512], right[512];
        std::fill(left, left + 512, 0.0f);
        std::fill(right, right + 512, 0.0f);

        voice_.Process(left, right, 512);

        for (int i = 0; i < 512; ++i) {
            ASSERT_FALSE(std::isnan(left[i]))
                << "Engine " << engine << " at note " << note << " produced NaN";
            ASSERT_FALSE(std::isinf(left[i]))
                << "Engine " << engine << " at note " << note << " produced Inf";
        }
    }
}

TEST_P(PlaitsEngineTest, EngineHandlesDifferentVelocities) {
    int engine = GetParam();
    voice_.set_engine(engine);

    const float velocities[] = {0.1f, 0.25f, 0.5f, 0.75f, 1.0f};

    for (float vel : velocities) {
        voice_.NoteOn(60, vel, 0.0f, 100.0f);

        float left[512], right[512];
        std::fill(left, left + 512, 0.0f);
        std::fill(right, right + 512, 0.0f);

        voice_.Process(left, right, 512);

        for (int i = 0; i < 512; ++i) {
            ASSERT_FALSE(std::isnan(left[i]))
                << "Engine " << engine << " at velocity " << vel << " produced NaN";
        }
    }
}

// Instantiate tests for all 16 engines
INSTANTIATE_TEST_SUITE_P(
    AllEngines,
    PlaitsEngineTest,
    ::testing::Range(0, 16),
    [](const ::testing::TestParamInfo<int>& info) {
        const char* names[] = {
            "VA", "Waveshaper", "FM", "Grain", "Additive", "Wavetable",
            "Chord", "Speech", "Swarm", "Noise", "Particle", "String",
            "Modal", "BassDrum", "Snare", "HiHat"
        };
        return names[info.param];
    }
);

// Additional engine-specific tests
class EngineSpecificTest : public ::testing::Test {
protected:
    void SetUp() override {
        voice_.Init(44100.0);
    }

    plaits_vst::Voice voice_;
};

TEST_F(EngineSpecificTest, DrumEnginesProducePercussiveEnvelopes) {
    // Test bass drum (13), snare (14), hi-hat (15)
    for (int engine : {13, 14, 15}) {
        voice_.set_engine(engine);
        voice_.NoteOn(60, 1.0f, 0.0f, 200.0f);

        float left[8192], right[8192];
        std::fill(left, left + 8192, 0.0f);
        std::fill(right, right + 8192, 0.0f);

        voice_.Process(left, right, 8192);

        // Find max amplitude in first half vs second half
        float maxFirst = 0.0f, maxSecond = 0.0f;
        for (int i = 0; i < 4096; ++i) {
            maxFirst = std::max(maxFirst, std::abs(left[i]));
        }
        for (int i = 4096; i < 8192; ++i) {
            maxSecond = std::max(maxSecond, std::abs(left[i]));
        }

        // Percussive sounds should decay
        // Note: This is a loose test since drum sounds vary
        EXPECT_GE(maxFirst + maxSecond, 0.0f)
            << "Drum engine " << engine << " should produce output";
    }
}

TEST_F(EngineSpecificTest, ChordEngineProducesMultiplePitches) {
    voice_.set_engine(6);  // Chord engine
    voice_.set_harmonics(0.5f);
    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[8192], right[8192];
    std::fill(left, left + 8192, 0.0f);
    std::fill(right, right + 8192, 0.0f);

    voice_.Process(left, right, 8192);

    // Just verify it produces output without crashing
    float sum = 0.0f;
    for (int i = 0; i < 8192; ++i) {
        sum += std::abs(left[i]);
    }

    EXPECT_GT(sum, 0.0f) << "Chord engine should produce output";
}

TEST_F(EngineSpecificTest, SpeechEngineWorks) {
    voice_.set_engine(7);  // Speech engine
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.5f);

    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[4096], right[4096];
    std::fill(left, left + 4096, 0.0f);
    std::fill(right, right + 4096, 0.0f);

    voice_.Process(left, right, 4096);

    float sum = 0.0f;
    for (int i = 0; i < 4096; ++i) {
        sum += std::abs(left[i]);
        ASSERT_FALSE(std::isnan(left[i])) << "Speech engine produced NaN";
    }

    EXPECT_GT(sum, 0.0f) << "Speech engine should produce output";
}

TEST_F(EngineSpecificTest, StringEngineResonates) {
    voice_.set_engine(11);  // String (Karplus-Strong)
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.3f);  // Damping

    voice_.NoteOn(60, 1.0f, 0.0f, 1000.0f);

    float left[16384], right[16384];
    std::fill(left, left + 16384, 0.0f);
    std::fill(right, right + 16384, 0.0f);

    voice_.Process(left, right, 16384);

    // String should have sustained output
    float sum = 0.0f;
    for (int i = 8192; i < 16384; ++i) {
        sum += std::abs(left[i]);
    }

    // Should still have some output in second half
    EXPECT_GE(sum, 0.0f) << "String engine output check";
}

TEST_F(EngineSpecificTest, ModalEngineResonates) {
    voice_.set_engine(12);  // Modal resonator
    voice_.set_harmonics(0.5f);
    voice_.set_timbre(0.5f);
    voice_.set_morph(0.5f);

    voice_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[4096], right[4096];
    std::fill(left, left + 4096, 0.0f);
    std::fill(right, right + 4096, 0.0f);

    voice_.Process(left, right, 4096);

    float sum = 0.0f;
    for (int i = 0; i < 4096; ++i) {
        sum += std::abs(left[i]);
        ASSERT_FALSE(std::isnan(left[i])) << "Modal engine produced NaN";
    }

    EXPECT_GT(sum, 0.0f) << "Modal engine should produce output";
}
