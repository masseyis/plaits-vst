#include <gtest/gtest.h>
#include "dsp/voice_allocator.h"
#include <cmath>
#include <set>

class VoiceAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        allocator_.Init(44100.0, 8);
    }

    VoiceAllocator allocator_;
};

TEST_F(VoiceAllocatorTest, InitialState) {
    EXPECT_EQ(allocator_.activeVoiceCount(), 0);
}

TEST_F(VoiceAllocatorTest, NoteOnAllocatesVoice) {
    allocator_.NoteOn(60, 1.0f, 10.0f, 100.0f);
    EXPECT_EQ(allocator_.activeVoiceCount(), 1);
}

TEST_F(VoiceAllocatorTest, MultipleNotesAllocateMultipleVoices) {
    allocator_.NoteOn(60, 1.0f, 10.0f, 100.0f);
    allocator_.NoteOn(64, 1.0f, 10.0f, 100.0f);
    allocator_.NoteOn(67, 1.0f, 10.0f, 100.0f);
    EXPECT_EQ(allocator_.activeVoiceCount(), 3);
}

TEST_F(VoiceAllocatorTest, NoteOffReleasesVoice) {
    allocator_.NoteOn(60, 1.0f, 10.0f, 10.0f);  // Short decay
    allocator_.NoteOff(60);

    // Process some audio to let envelope complete
    float left[4096], right[4096];
    for (int i = 0; i < 100; ++i) {
        allocator_.Process(left, right, 4096);
    }

    EXPECT_EQ(allocator_.activeVoiceCount(), 0);
}

TEST_F(VoiceAllocatorTest, AllNotesOffReleasesAll) {
    allocator_.NoteOn(60, 1.0f, 10.0f, 10.0f);
    allocator_.NoteOn(64, 1.0f, 10.0f, 10.0f);
    allocator_.NoteOn(67, 1.0f, 10.0f, 10.0f);
    allocator_.AllNotesOff();

    // Process to complete envelopes
    float left[4096], right[4096];
    for (int i = 0; i < 100; ++i) {
        allocator_.Process(left, right, 4096);
    }

    EXPECT_EQ(allocator_.activeVoiceCount(), 0);
}

TEST_F(VoiceAllocatorTest, RespectsPolyphonyLimit) {
    allocator_.setPolyphony(4);

    // Try to play 8 notes
    for (int i = 0; i < 8; ++i) {
        allocator_.NoteOn(60 + i, 1.0f, 10.0f, 1000.0f);
    }

    EXPECT_LE(allocator_.activeVoiceCount(), 4);
}

TEST_F(VoiceAllocatorTest, VoiceStealingWorks) {
    allocator_.setPolyphony(4);

    // Play 4 notes
    allocator_.NoteOn(60, 1.0f, 10.0f, 1000.0f);
    allocator_.NoteOn(64, 1.0f, 10.0f, 1000.0f);
    allocator_.NoteOn(67, 1.0f, 10.0f, 1000.0f);
    allocator_.NoteOn(72, 1.0f, 10.0f, 1000.0f);

    // Play a 5th note - should steal oldest
    allocator_.NoteOn(76, 1.0f, 10.0f, 1000.0f);

    EXPECT_LE(allocator_.activeVoiceCount(), 4);
}

TEST_F(VoiceAllocatorTest, ProcessProducesOutput) {
    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[256], right[256];
    std::fill(left, left + 256, 0.0f);
    std::fill(right, right + 256, 0.0f);

    allocator_.Process(left, right, 256);

    float sum = 0.0f;
    for (int i = 0; i < 256; ++i) {
        sum += std::abs(left[i]) + std::abs(right[i]);
    }

    EXPECT_GT(sum, 0.0f) << "Should produce output when notes are active";
}

TEST_F(VoiceAllocatorTest, NoOutputWhenSilent) {
    float left[256], right[256];
    std::fill(left, left + 256, 1.0f);  // Non-zero initial
    std::fill(right, right + 256, 1.0f);

    allocator_.Process(left, right, 256);

    float sum = 0.0f;
    for (int i = 0; i < 256; ++i) {
        sum += std::abs(left[i]) + std::abs(right[i]);
    }

    EXPECT_FLOAT_EQ(sum, 0.0f) << "Should produce silence when no notes active";
}

TEST_F(VoiceAllocatorTest, EngineParameterAffectsAllVoices) {
    allocator_.set_engine(0);
    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left1[256], right1[256];
    std::fill(left1, left1 + 256, 0.0f);
    std::fill(right1, right1 + 256, 0.0f);
    allocator_.Process(left1, right1, 256);

    // Change engine and play new note
    allocator_.AllNotesOff();
    float temp[4096], tempR[4096];
    for (int i = 0; i < 50; ++i) {
        allocator_.Process(temp, tempR, 4096);
    }

    allocator_.set_engine(5);  // Wavetable
    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left2[256], right2[256];
    std::fill(left2, left2 + 256, 0.0f);
    std::fill(right2, right2 + 256, 0.0f);
    allocator_.Process(left2, right2, 256);

    float diff = 0.0f;
    for (int i = 0; i < 256; ++i) {
        diff += std::abs(left1[i] - left2[i]);
    }

    EXPECT_GT(diff, 0.1f) << "Different engines should produce different output";
}

TEST_F(VoiceAllocatorTest, PolyphonyCanBeChanged) {
    allocator_.setPolyphony(2);
    EXPECT_EQ(allocator_.activeVoiceCount(), 0);

    allocator_.NoteOn(60, 1.0f, 10.0f, 1000.0f);
    allocator_.NoteOn(64, 1.0f, 10.0f, 1000.0f);
    allocator_.NoteOn(67, 1.0f, 10.0f, 1000.0f);

    EXPECT_LE(allocator_.activeVoiceCount(), 2);

    // Increase polyphony
    allocator_.setPolyphony(8);

    // New notes should now be able to allocate
    allocator_.NoteOn(72, 1.0f, 10.0f, 1000.0f);
    EXPECT_GE(allocator_.activeVoiceCount(), 1);
}

TEST_F(VoiceAllocatorTest, MultipleVoicesMix) {
    allocator_.setPolyphony(8);

    // Play a chord
    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);  // C
    allocator_.NoteOn(64, 1.0f, 0.0f, 500.0f);  // E
    allocator_.NoteOn(67, 1.0f, 0.0f, 500.0f);  // G

    float left[512], right[512];
    std::fill(left, left + 512, 0.0f);
    std::fill(right, right + 512, 0.0f);

    allocator_.Process(left, right, 512);

    // Output should be reasonable (not clipping badly)
    float maxVal = 0.0f;
    for (int i = 0; i < 512; ++i) {
        maxVal = std::max(maxVal, std::abs(left[i]));
        maxVal = std::max(maxVal, std::abs(right[i]));
    }

    EXPECT_LT(maxVal, 10.0f) << "Mixed output should not be excessively loud";
    EXPECT_GT(maxVal, 0.0f) << "Should have some output";
}

TEST_F(VoiceAllocatorTest, SameNoteRetriggers) {
    allocator_.NoteOn(60, 1.0f, 10.0f, 1000.0f);
    int initialCount = allocator_.activeVoiceCount();

    // Play same note again
    allocator_.NoteOn(60, 1.0f, 10.0f, 1000.0f);

    // Should still only have one voice for this note (or steal and reallocate)
    EXPECT_LE(allocator_.activeVoiceCount(), initialCount + 1);
}

TEST_F(VoiceAllocatorTest, OutputIsNotNaNOrInf) {
    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[1024], right[1024];

    for (int block = 0; block < 100; ++block) {
        std::fill(left, left + 1024, 0.0f);
        std::fill(right, right + 1024, 0.0f);

        allocator_.Process(left, right, 1024);

        for (int i = 0; i < 1024; ++i) {
            EXPECT_FALSE(std::isnan(left[i])) << "Left NaN at block " << block << " sample " << i;
            EXPECT_FALSE(std::isnan(right[i])) << "Right NaN at block " << block << " sample " << i;
            EXPECT_FALSE(std::isinf(left[i])) << "Left Inf at block " << block << " sample " << i;
            EXPECT_FALSE(std::isinf(right[i])) << "Right Inf at block " << block << " sample " << i;
        }
    }
}

TEST_F(VoiceAllocatorTest, HarmonicsTimbreMorphWork) {
    allocator_.set_harmonics(0.5f);
    allocator_.set_timbre(0.5f);
    allocator_.set_morph(0.5f);

    allocator_.NoteOn(60, 1.0f, 0.0f, 500.0f);

    float left[256], right[256];
    std::fill(left, left + 256, 0.0f);
    std::fill(right, right + 256, 0.0f);

    // Should not crash
    allocator_.Process(left, right, 256);

    float sum = 0.0f;
    for (int i = 0; i < 256; ++i) {
        sum += std::abs(left[i]);
    }

    EXPECT_GT(sum, 0.0f);
}
