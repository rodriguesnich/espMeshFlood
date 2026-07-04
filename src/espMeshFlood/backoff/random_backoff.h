#ifndef ESPMESHFLOOD_BACKOFF_RANDOM_BACKOFF_H
#define ESPMESHFLOOD_BACKOFF_RANDOM_BACKOFF_H

#include <cstdint>

namespace espMeshFlood {

/**
 * @brief Random backoff generator for retransmission delays
 * 
 * Generates random delays within a configured range to reduce collisions
 * during simultaneous retransmissions. Supports seeding for deterministic
 * testing.
 */
class RandomBackoff {
public:
    /**
     * @brief Initialize backoff with delay range
     * @param min_delay_ms Minimum delay in milliseconds
     * @param max_delay_ms Maximum delay in milliseconds
     * @param seed Optional seed for deterministic behavior (0 = use system randomness)
     */
    RandomBackoff(uint32_t min_delay_ms, uint32_t max_delay_ms, uint32_t seed = 0);

    /**
     * @brief Get a random delay within configured range
     * @return Random delay in milliseconds
     */
    uint32_t get_delay();

    /**
     * @brief Set seed for deterministic randomization
     * @param seed Random seed value
     */
    void set_seed(uint32_t seed);

private:
    uint32_t min_delay_ms_;
    uint32_t max_delay_ms_;
    uint32_t state_;
    bool use_system_random_;

    /**
     * @brief Simple Linear Congruential Generator (LCG) for embedded systems
     * @return Next pseudo-random number
     */
    uint32_t next_random();
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_BACKOFF_RANDOM_BACKOFF_H
