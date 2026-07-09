// Minimal example stub for builds when example folders are not present.
// This provides a safe default for both ESP32 (Arduino) and native builds.

#ifdef ESP32
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("[espMeshFlood] Example stub: no examples provided in repository.");
}

void loop() {
    // Perform any minimal background tasks if needed
    delay(1000);
}

#else

int main() {
    // No-op for native builds; tests and examples run separately.
    return 0;
}

#endif