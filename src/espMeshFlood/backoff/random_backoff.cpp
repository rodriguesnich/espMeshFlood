#include "espMeshFlood/backoff/random_backoff.h"
#include <ctime>
#include <cstdlib>

namespace espMeshFlood {

RandomBackoff::RandomBackoff(uint32_t min_delay_ms, uint32_t max_delay_ms, uint32_t seed)
    : min_delay_ms_(min_delay_ms), max_delay_ms_(max_delay_ms), state_(seed), use_system_random_(seed == 0) {
    if (use_system_random_) {
        // Seed with current time if seed is 0
        state_ = static_cast<uint32_t>(std::time(nullptr));
    }
}

uint32_t RandomBackoff::get_delay() {
    if (use_system_random_) {
        // Use system random (e.g., esp_random() on ESP32)
        state_ = std::rand();
    } else {
        // Use seeded LCG for deterministic tests
        state_ = next_random();
    }

    uint32_t range = max_delay_ms_ - min_delay_ms_;
    if (range == 0) {
        return min_delay_ms_;
    }

    uint32_t random_offset = (state_ % (range + 1));
    return min_delay_ms_ + random_offset;
}

void RandomBackoff::set_seed(uint32_t seed) {
    state_ = seed;
    use_system_random_ = (seed == 0);
}

uint32_t RandomBackoff::next_random() {
    // Linear Congruential Generator (LCG)
    // a = 1103515245, c = 12345, m = 2^31
    state_ = (1103515245U * state_ + 12345U) & 0x7fffffffU;
    return state_;
}

}  // namespace espMeshFlood
