#include <gtest/gtest.h>
#include "espMeshFlood/backoff/random_backoff.h"
#include <set>

using namespace espMeshFlood;

class RandomBackoffTest : public ::testing::Test {
protected:
    void SetUp() override {
        backoff = std::make_unique<RandomBackoff>(50, 300, 1);  // Seeded for deterministic tests
    }

    std::unique_ptr<RandomBackoff> backoff;
};

TEST_F(RandomBackoffTest, DelayInRange) {
    for (int i = 0; i < 100; i++) {
        uint32_t delay = backoff->get_delay();
        EXPECT_GE(delay, 50);
        EXPECT_LE(delay, 300);
    }
}

TEST_F(RandomBackoffTest, DelayVariation) {
    // With seeding, we should get different values
    std::set<uint32_t> delays;
    
    RandomBackoff seeded_backoff(50, 300, 42);
    for (int i = 0; i < 20; i++) {
        delays.insert(seeded_backoff.get_delay());
    }
    
    // We expect at least some variation (not all the same value)
    EXPECT_GT(delays.size(), 1);
}

TEST_F(RandomBackoffTest, ZeroRangeReturnsMin) {
    RandomBackoff zero_range(100, 100, 1);
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(zero_range.get_delay(), 100);
    }
}

TEST_F(RandomBackoffTest, SeededDeterminism) {
    RandomBackoff backoff1(50, 300, 12345);
    RandomBackoff backoff2(50, 300, 12345);
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(backoff1.get_delay(), backoff2.get_delay());
    }
}

TEST_F(RandomBackoffTest, DifferentSeedsDifferentSequence) {
    RandomBackoff backoff1(50, 300, 111);
    RandomBackoff backoff2(50, 300, 222);
    
    bool any_different = false;
    for (int i = 0; i < 10; i++) {
        if (backoff1.get_delay() != backoff2.get_delay()) {
            any_different = true;
            break;
        }
    }
    
    EXPECT_TRUE(any_different);
}

TEST_F(RandomBackoffTest, SetSeedAffectsSequence) {
    RandomBackoff backoff1(50, 300, 999);
    uint32_t val1 = backoff1.get_delay();
    uint32_t val2 = backoff1.get_delay();
    
    // Reset with same seed
    backoff1.set_seed(999);
    EXPECT_EQ(backoff1.get_delay(), val1);
    EXPECT_EQ(backoff1.get_delay(), val2);
}

TEST_F(RandomBackoffTest, SmallRange) {
    RandomBackoff small_backoff(100, 105, 50);
    
    for (int i = 0; i < 20; i++) {
        uint32_t delay = small_backoff.get_delay();
        EXPECT_GE(delay, 100);
        EXPECT_LE(delay, 105);
    }
}

TEST_F(RandomBackoffTest, LargeRange) {
    RandomBackoff large_backoff(0, 1000, 77);

    for (int i = 0; i < 20; i++) {
        uint32_t delay = large_backoff.get_delay();
        // delay is unsigned, so lower bound is implicit.
        EXPECT_LE(delay, 1000);
    }
}
