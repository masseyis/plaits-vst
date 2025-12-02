// ModEnvelope Tests
// Part of PlaitsVST - MIT License

#include <gtest/gtest.h>
#include "dsp/mod_envelope.h"
#include <cmath>

using namespace plaits;

class ModEnvelopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        env_.Init();
    }

    ModEnvelope env_;
    static constexpr float kSampleRate = 48000.0f;
};

TEST_F(ModEnvelopeTest, InitDoesNotCrash) {
    EXPECT_EQ(env_.GetAttack(), 10);
    EXPECT_EQ(env_.GetDecay(), 200);
    EXPECT_FALSE(env_.IsActive());
}

TEST_F(ModEnvelopeTest, StartsIdle) {
    EXPECT_FALSE(env_.IsActive());
    EXPECT_TRUE(env_.IsComplete());
    EXPECT_FLOAT_EQ(env_.GetOutput(), 0.0f);
}

TEST_F(ModEnvelopeTest, TriggerActivates) {
    env_.Trigger();
    EXPECT_TRUE(env_.IsActive());
    EXPECT_FALSE(env_.IsComplete());
}

TEST_F(ModEnvelopeTest, AttackRises) {
    env_.SetAttack(50);  // 50ms attack
    env_.Trigger();

    float initial = env_.GetOutput();

    // Process through attack phase
    for (int i = 0; i < 2400; ++i) {  // 50ms at 48kHz
        env_.Process(kSampleRate);
    }

    float after_attack = env_.GetOutput();

    // Output should have risen from initial value
    EXPECT_GT(after_attack, initial);
}

TEST_F(ModEnvelopeTest, ReachesPeak) {
    env_.SetAttack(10);
    env_.SetDecay(100);
    env_.Trigger();

    float max_val = 0.0f;

    // Process through attack and into decay
    for (int i = 0; i < 10000; ++i) {
        float output = env_.Process(kSampleRate);
        max_val = std::max(max_val, output);
    }

    // Should reach close to 1.0 at peak
    EXPECT_GT(max_val, 0.95f);
}

TEST_F(ModEnvelopeTest, DecayFalls) {
    env_.SetAttack(5);
    env_.SetDecay(100);
    env_.Trigger();

    // Get through attack
    for (int i = 0; i < 500; ++i) {
        env_.Process(kSampleRate);
    }

    float peak = env_.GetOutput();

    // Process decay
    for (int i = 0; i < 5000; ++i) {
        env_.Process(kSampleRate);
    }

    float after_decay = env_.GetOutput();
    EXPECT_LT(after_decay, peak);
}

TEST_F(ModEnvelopeTest, EventuallyCompletes) {
    env_.SetAttack(10);
    env_.SetDecay(50);
    env_.Trigger();

    // Process until complete
    int samples = 0;
    while (!env_.IsComplete() && samples < 100000) {
        env_.Process(kSampleRate);
        samples++;
    }

    EXPECT_TRUE(env_.IsComplete());
    EXPECT_LT(samples, 100000);  // Should complete in reasonable time
}

TEST_F(ModEnvelopeTest, OutputInRange) {
    env_.SetAttack(20);
    env_.SetDecay(100);
    env_.Trigger();

    for (int i = 0; i < 20000; ++i) {
        float output = env_.Process(kSampleRate);
        EXPECT_GE(output, 0.0f);
        EXPECT_LE(output, 1.0f);
    }
}

TEST_F(ModEnvelopeTest, ResetWorks) {
    env_.Trigger();

    // Process a bit
    for (int i = 0; i < 1000; ++i) {
        env_.Process(kSampleRate);
    }

    env_.Reset();
    EXPECT_FALSE(env_.IsActive());
    EXPECT_FLOAT_EQ(env_.GetOutput(), 0.0f);
}

TEST_F(ModEnvelopeTest, RetriggerDuringAttack) {
    env_.SetAttack(50);
    env_.SetDecay(200);
    env_.Trigger();

    // Process partway through attack
    for (int i = 0; i < 1000; ++i) {
        env_.Process(kSampleRate);
    }

    float mid_attack = env_.GetOutput();
    EXPECT_GT(mid_attack, 0.0f);
    EXPECT_LT(mid_attack, 1.0f);

    // Retrigger
    env_.Trigger();
    EXPECT_TRUE(env_.IsActive());
}
