#!/bin/bash
# Build and run EFMP tests locally

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "=== EFMP Test Build and Execution ==="
echo "Building test suite..."

# Compile test suite directly
g++ -std=c++17 \
    -I./src \
    -I./test \
    -Wall -Wextra \
    \
    ./test/unit/cache_test.cpp \
    ./test/unit/ttl_test.cpp \
    ./test/unit/backoff_test.cpp \
    ./test/unit/serialization_test.cpp \
    ./test/integration/message_flow_test.cpp \
    \
    ./src/espMeshFlood/cache/message_cache.cpp \
    ./src/espMeshFlood/message/message_metadata.cpp \
    ./src/espMeshFlood/backoff/random_backoff.cpp \
    ./src/espMeshFlood/protocol/mesh_protocol.cpp \
    ./src/espMeshFlood/serialization/message_serializer.cpp \
    ./src/espMeshFlood/efmp.cpp \
    \
    -o build/test_runner \
    -lgtest -lgtest_main -lpthread 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "=== Running Tests ==="
    ./build/test_runner
    TEST_RESULT=$?
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo ""
        echo "✓ ALL TESTS PASSED"
        exit 0
    else
        echo ""
        echo "✗ Some tests failed (exit code: $TEST_RESULT)"
        exit 1
    fi
else
    echo "✗ Compilation failed"
    exit 1
fi
