#include <Arduino.h>
#include <espMeshFlood/efmp.h>
#include <esp_now.h>

using namespace espMeshFlood;

// Global timing
unsigned long last_send_time = 0;
const unsigned long SEND_INTERVAL_MS = 5000;  // Send every 5 seconds
unsigned long last_maintenance_time = 0;
const unsigned long MAINTENANCE_INTERVAL_MS = 1000;  // Maintenance every 1 second

// Statistics
uint32_t messages_sent = 0;
uint32_t messages_received = 0;

/**
 * @brief Callback when a message is received
 */
void on_mesh_message_received(const MeshMessage& msg, int32_t rssi) {
    messages_received++;
    
    Serial.printf("[MESH] Received from node 0x%X:\n", msg.sender_id);
    Serial.printf("  Message ID: 0x%llX\n", msg.message_id);
    Serial.printf("  TTL: %u, Relay count: %u\n", msg.ttl, msg.relay_count);
    Serial.printf("  Payload size: %zu bytes\n", msg.payload.size());
    Serial.printf("  RSSI: %d dBm\n", rssi);
    Serial.printf("  Stats - Sent: %u, Received: %u\n", messages_sent, messages_received);
}

/**
 * @brief Initialize the mesh network
 */
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ESP Flood Mesh Protocol (EFMP) Demo ===\n");
    
    // Initialize WiFi for ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.printf("Node MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Node ID: 0x%08X\n\n", EspMeshFlood::instance().get_node_id());
    
    // Initialize EFMP
    if (!EspMeshFlood::instance().init(on_mesh_message_received)) {
        Serial.println("ERROR: Failed to initialize EFMP!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("EFMP initialized successfully!");
    Serial.println("\nWaiting for mesh messages...\n");
}

/**
 * @brief Main loop - send periodic messages and maintain the mesh
 */
void loop() {
    unsigned long now = millis();
    
    // Periodic message sending
    if (now - last_send_time >= SEND_INTERVAL_MS) {
        last_send_time = now;
        messages_sent++;
        
        // Create a simple payload with counter
        uint8_t payload[20];
        snprintf((char*)payload, sizeof(payload), "Hello %u", messages_sent);
        
        Serial.printf("[SEND] Sending message #%u...\n", messages_sent);
        if (!EspMeshFlood::instance().send_message(payload, strlen((char*)payload) + 1, 5)) {
            Serial.println("  ERROR: Send failed!");
        } else {
            Serial.println("  OK");
        }
    }
    
    // Periodic maintenance
    if (now - last_maintenance_time >= MAINTENANCE_INTERVAL_MS) {
        last_maintenance_time = now;
        
        // Update system time and perform maintenance
        EspMeshFlood::instance().update_time(now);
        EspMeshFlood::instance().do_maintenance();
    }
    
    delay(100);
}

/**
 * @brief CLI commands for testing
 * Type 's' to send a test message
 * Type 'h' to display help
 */
void serialEvent() {
    while (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 's':
            case 'S':
                {
                    messages_sent++;
                    uint8_t payload[] = "Test message";
                    EspMeshFlood::instance().send_message(payload, sizeof(payload), 5);
                    Serial.printf("[USER] Sent test message #%u\n", messages_sent);
                }
                break;
            
            case 'h':
            case 'H':
                Serial.println("\n=== EFMP Demo Commands ===");
                Serial.println("  's' - Send a test message");
                Serial.println("  'h' - Show this help");
                Serial.println("  Any message sent will be relayed by the mesh network\n");
                break;
            
            default:
                break;
        }
    }
}
