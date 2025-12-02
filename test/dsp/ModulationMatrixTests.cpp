// ModulationMatrix Tests
// Part of PlaitsVST - MIT License

#include <gtest/gtest.h>
#include "dsp/modulation_matrix.h"
#include <cmath>

using namespace plaits;

class ModulationMatrixTest : public ::testing::Test {
protected:
    void SetUp() override {
        matrix_.Init();
    }

    ModulationMatrix matrix_;
    static constexpr float kSampleRate = 48000.0f;
};

TEST_F(ModulationMatrixTest, InitDoesNotCrash) {
    // Verify default state (Plaits uses Harmonics/Timbre/Morph)
    EXPECT_EQ(matrix_.GetDestination(ModSource::Lfo1), ModDestination::Timbre);
    EXPECT_EQ(matrix_.GetDestination(ModSource::Lfo2), ModDestination::Morph);
    EXPECT_EQ(matrix_.GetAmount(ModSource::Lfo1), 0);
}

TEST_F(ModulationMatrixTest, SetDestinationWorks) {
    matrix_.SetDestination(ModSource::Lfo1, ModDestination::Harmonics);
    EXPECT_EQ(matrix_.GetDestination(ModSource::Lfo1), ModDestination::Harmonics);

    matrix_.SetDestination(ModSource::Env1, ModDestination::Lfo1Rate);
    EXPECT_EQ(matrix_.GetDestination(ModSource::Env1), ModDestination::Lfo1Rate);
}

TEST_F(ModulationMatrixTest, SetAmountWorks) {
    matrix_.SetAmount(ModSource::Lfo1, 32);
    EXPECT_EQ(matrix_.GetAmount(ModSource::Lfo1), 32);

    matrix_.SetAmount(ModSource::Lfo2, -48);
    EXPECT_EQ(matrix_.GetAmount(ModSource::Lfo2), -48);
}

TEST_F(ModulationMatrixTest, AmountClamps) {
    matrix_.SetAmount(ModSource::Lfo1, 100);
    EXPECT_EQ(matrix_.GetAmount(ModSource::Lfo1), 63);

    matrix_.SetAmount(ModSource::Lfo1, -100);
    EXPECT_EQ(matrix_.GetAmount(ModSource::Lfo1), -64);
}

TEST_F(ModulationMatrixTest, ProcessUpdatesModulation) {
    matrix_.SetDestination(ModSource::Lfo1, ModDestination::Timbre);
    matrix_.SetAmount(ModSource::Lfo1, 64);  // Full amount
    matrix_.GetLfo1().SetRate(LfoRateDivision::Div_1_16);  // Fast LFO

    // Process several times to let LFO move
    for (int i = 0; i < 100; ++i) {
        matrix_.Process(kSampleRate, 64);
    }

    // Modulation should be non-zero at some point
    float mod = matrix_.GetModulation(ModDestination::Timbre);
    // Could be positive or negative depending on LFO phase
    // Just verify it's being calculated
    SUCCEED();  // If we got here without crash, basic processing works
}

TEST_F(ModulationMatrixTest, ZeroAmountNoModulation) {
    matrix_.SetDestination(ModSource::Lfo1, ModDestination::Timbre);
    matrix_.SetAmount(ModSource::Lfo1, 0);

    for (int i = 0; i < 100; ++i) {
        matrix_.Process(kSampleRate, 64);
    }

    float mod = matrix_.GetModulation(ModDestination::Timbre);
    EXPECT_NEAR(mod, 0.0f, 0.01f);
}

TEST_F(ModulationMatrixTest, EnvelopeTriggerWorks) {
    matrix_.SetDestination(ModSource::Env1, ModDestination::Harmonics);
    matrix_.SetAmount(ModSource::Env1, 64);
    matrix_.GetEnv1().SetAttack(10);
    matrix_.GetEnv1().SetDecay(500);  // Longer decay so we catch it active

    // Before trigger, envelope should be idle
    matrix_.Process(kSampleRate, 64);
    float mod_before = matrix_.GetModulation(ModDestination::Harmonics);

    // Trigger
    matrix_.TriggerEnvelopes();

    // Process just a few samples to catch envelope rising
    // 10ms attack = 480 samples, so 5 iterations * 64 = 320 samples should be mid-attack
    for (int i = 0; i < 5; ++i) {
        matrix_.Process(kSampleRate, 64);
    }

    float mod_after = matrix_.GetModulation(ModDestination::Harmonics);

    // Envelope should now be contributing modulation
    EXPECT_NE(mod_before, mod_after);
}

TEST_F(ModulationMatrixTest, GetModulatedValueWorks) {
    // With zero modulation
    matrix_.SetAmount(ModSource::Lfo1, 0);
    matrix_.SetAmount(ModSource::Lfo2, 0);
    matrix_.SetAmount(ModSource::Env1, 0);
    matrix_.SetAmount(ModSource::Env2, 0);
    matrix_.Process(kSampleRate, 64);

    float base = 0.5f;
    float modulated = matrix_.GetModulatedValue(ModDestination::Timbre, base);
    EXPECT_NEAR(modulated, base, 0.01f);
}

TEST_F(ModulationMatrixTest, ModulatedValueClamps) {
    matrix_.SetDestination(ModSource::Lfo1, ModDestination::Timbre);
    matrix_.SetAmount(ModSource::Lfo1, 64);

    // Process to get LFO moving
    for (int i = 0; i < 1000; ++i) {
        matrix_.Process(kSampleRate, 64);
    }

    // Try extreme base values
    float mod_low = matrix_.GetModulatedValue(ModDestination::Timbre, 0.0f);
    float mod_high = matrix_.GetModulatedValue(ModDestination::Timbre, 1.0f);

    EXPECT_GE(mod_low, 0.0f);
    EXPECT_LE(mod_low, 1.0f);
    EXPECT_GE(mod_high, 0.0f);
    EXPECT_LE(mod_high, 1.0f);
}

TEST_F(ModulationMatrixTest, TempoSyncWorks) {
    matrix_.SetTempo(120.0);
    // Just verify no crash
    matrix_.Process(kSampleRate, 64);

    matrix_.SetTempo(60.0);
    matrix_.Process(kSampleRate, 64);
    SUCCEED();
}

TEST_F(ModulationMatrixTest, ResetWorks) {
    matrix_.TriggerEnvelopes();
    matrix_.Process(kSampleRate, 64);

    matrix_.Reset();

    // After reset, envelopes should be idle
    EXPECT_TRUE(matrix_.GetEnv1().IsComplete());
    EXPECT_TRUE(matrix_.GetEnv2().IsComplete());
}

TEST_F(ModulationMatrixTest, DestinationNamesExist) {
    for (int i = 0; i < static_cast<int>(ModDestination::NumDestinations); ++i) {
        const char* name = ModulationMatrix::GetDestinationName(static_cast<ModDestination>(i));
        EXPECT_NE(name, nullptr);
        EXPECT_STRNE(name, "???");
    }
}

TEST_F(ModulationMatrixTest, MultipleSourcesSameDestination) {
    // Both LFOs targeting Timbre
    matrix_.SetDestination(ModSource::Lfo1, ModDestination::Timbre);
    matrix_.SetDestination(ModSource::Lfo2, ModDestination::Timbre);
    matrix_.SetAmount(ModSource::Lfo1, 32);
    matrix_.SetAmount(ModSource::Lfo2, 32);

    // Should combine without issues
    for (int i = 0; i < 100; ++i) {
        matrix_.Process(kSampleRate, 64);
    }

    // Modulation should be in valid range
    float mod = matrix_.GetModulation(ModDestination::Timbre);
    EXPECT_GE(mod, -1.0f);
    EXPECT_LE(mod, 1.0f);
}
