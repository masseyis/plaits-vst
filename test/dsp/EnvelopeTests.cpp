#include <gtest/gtest.h>
#include "dsp/envelope.h"

class EnvelopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        env_.Init(44100.0);
    }

    Envelope env_;
};

TEST_F(EnvelopeTest, InitialState) {
    EXPECT_FALSE(env_.active());
    EXPECT_TRUE(env_.done());
    EXPECT_FLOAT_EQ(env_.Process(), 0.0f);
}

TEST_F(EnvelopeTest, TriggerStartsEnvelope) {
    env_.Trigger(10.0f, 100.0f);  // 10ms attack, 100ms decay
    EXPECT_TRUE(env_.active());
    EXPECT_FALSE(env_.done());
}

TEST_F(EnvelopeTest, AttackPhaseRises) {
    env_.Trigger(10.0f, 100.0f);

    float prevValue = 0.0f;
    // Process through attack phase (10ms at 44100 = ~441 samples)
    for (int i = 0; i < 200; ++i) {
        float value = env_.Process();
        EXPECT_GE(value, prevValue) << "Attack should rise monotonically";
        prevValue = value;
    }
    EXPECT_GT(prevValue, 0.0f);
}

TEST_F(EnvelopeTest, ReachesPeakAndDecays) {
    env_.Trigger(5.0f, 50.0f);  // Short envelope

    // Find peak
    float peak = 0.0f;
    int peakSample = 0;
    for (int i = 0; i < 10000; ++i) {
        float value = env_.Process();
        if (value > peak) {
            peak = value;
            peakSample = i;
        }
    }

    EXPECT_GT(peak, 0.9f) << "Should reach near full level";
    EXPECT_GT(peakSample, 0) << "Peak should not be at sample 0";
}

TEST_F(EnvelopeTest, EnvelopeCompletes) {
    env_.Trigger(1.0f, 10.0f);  // Very short envelope

    // Process until done
    int samples = 0;
    while (!env_.done() && samples < 100000) {
        env_.Process();
        samples++;
    }

    EXPECT_TRUE(env_.done()) << "Envelope should complete";
    EXPECT_LT(samples, 100000) << "Should complete in reasonable time";
}

TEST_F(EnvelopeTest, RetriggerResetsEnvelope) {
    env_.Trigger(10.0f, 100.0f);

    // Process halfway through
    for (int i = 0; i < 1000; ++i) {
        env_.Process();
    }

    // Retrigger
    env_.Trigger(10.0f, 100.0f);
    EXPECT_TRUE(env_.active());
    EXPECT_FALSE(env_.done());
}

TEST_F(EnvelopeTest, ZeroAttackStartsAtPeak) {
    env_.Trigger(0.0f, 100.0f);

    float firstValue = env_.Process();
    EXPECT_GT(firstValue, 0.9f) << "Zero attack should start near peak";
}

TEST_F(EnvelopeTest, LongDecayTakesLonger) {
    Envelope shortEnv, longEnv;
    shortEnv.Init(44100.0);
    longEnv.Init(44100.0);

    shortEnv.Trigger(1.0f, 50.0f);
    longEnv.Trigger(1.0f, 500.0f);

    int shortSamples = 0, longSamples = 0;

    while (!shortEnv.done() && shortSamples < 100000) {
        shortEnv.Process();
        shortSamples++;
    }

    while (!longEnv.done() && longSamples < 100000) {
        longEnv.Process();
        longSamples++;
    }

    EXPECT_GT(longSamples, shortSamples * 2) << "Longer decay should take more samples";
}
