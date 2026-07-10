// Minimal example showing basic usage of the espMeshFlood API.
// Works on ESP32 (Arduino) and native builds (uses mock transport).

#ifdef ESP32
#include <Arduino.h>
#include "espMeshFlood/efmp.h"
#include "espMeshFlood/types.h"

void on_message_received(const espMeshFlood::MeshMessage& message, int32_t rssi) {
    Serial.print("[espMeshFlood] Received message from 0x");
    Serial.print(message.origin_node_id, HEX);
    Serial.print(" rssi=");
    Serial.print(rssi);
    Serial.print(" : ");
    for (size_t i = 0; i < message.payload.size(); ++i) Serial.print((char)message.payload[i]);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[espMeshFlood] Minimal example (ESP32)");

    // Initialize library with a receive callback; runtime transport is automatic on ESP32.
    bool ok = espMeshFlood::EspMeshFlood::instance().init(on_message_received);
    Serial.print("Init: ");
    Serial.println(ok ? "OK" : "FAIL");
}

void loop() {
    // Periodic maintenance (non-blocking).
    espMeshFlood::EspMeshFlood::instance().do_maintenance();
    delay(1000);
}

#else

#include <iostream>
#include <chrono>
#include <thread>
#include "espMeshFlood/efmp.h"

// Native build: prints framework info, initializes library with mock transport,
// sends a single test broadcast and exits.
void on_message_received(const espMeshFlood::MeshMessage& message, int32_t rssi) {
    std::cout << "[espMeshFlood] Received from 0x" << std::hex << message.origin_node_id << std::dec << " rssi=" << rssi << " : ";
    std::cout.write(reinterpret_cast<const char*>(message.payload.data()), static_cast<std::streamsize>(message.payload.size()));
    std::cout << "\n";
}

int main() {
    std::cout << "[espMeshFlood] Minimal native example" << std::endl;

    auto& lib = espMeshFlood::EspMeshFlood::instance();
    if (!lib.init(on_message_received)) {
        std::cerr << "Init failed\n";
        return 1;
    }

    const char msg[] = "hello from native example";
    bool sent = lib.send_message(reinterpret_cast<const uint8_t*>(msg), sizeof(msg) - 1, 5);
    std::cout << "send_message: " << (sent ? "OK" : "FAIL") << std::endl;

    // Let mock transport deliver callbacks (if any) and run maintenance a few times.
    for (int i = 0; i < 5; ++i) {
        lib.do_maintenance();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    lib.deinit();
    std::cout << "Done." << std::endl;
    return 0;
}

#endif