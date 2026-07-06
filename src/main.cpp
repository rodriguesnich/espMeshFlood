// Build shim for PlatformIO: compile the example sketch as the firmware entrypoint.
#ifdef ESP32
#include "../examples/minimal_mesh_node/main.cpp"
#endif