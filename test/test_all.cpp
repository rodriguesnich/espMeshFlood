// Aggregated test suite for EFMP
// This file includes all test cases and allows PlatformIO native test framework
// to discover and execute them properly.

#include <gtest/gtest.h>

// Include all unit test cases
#include "unit/cache_test.cpp"
#include "unit/ttl_test.cpp"
#include "unit/backoff_test.cpp"


// Include integration test cases
#include "integration/message_flow_test.cpp"

// googletest main() is automatically provided by the framework
