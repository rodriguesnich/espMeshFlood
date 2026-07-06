#include <Arduino.h>
#include <espMeshFlood/efmp.h>
#include <espMeshFlood/types.h>
#include <espMeshFlood/serialization/message_serializer.h>
#include <esp_now.h>
#include <cstring>

using namespace espMeshFlood;

// Frame constants
const uint8_t MAGIC_BYTE = 0xAA;  // Magic byte for frame synchronization
const size_t MAGIC_SIZE = 1;
const size_t LENGTH_SIZE = 2;     // 16-bit length field
const size_t FRAME_HEADER_SIZE = MAGIC_SIZE + LENGTH_SIZE;

// Global timing
unsigned long last_send_time = 0;
const unsigned long SEND_INTERVAL_MS = 5000;  // Send every 5 seconds
unsigned long last_maintenance_time = 0;
const unsigned long MAINTENANCE_INTERVAL_MS = 1000;  // Maintenance every 1 second

// Statistics
uint32_t messages_sent = 0;
uint32_t messages_received = 0;

/**
 * @brief Send a framed serial message
 * @param payload Protobuf serialized data
 * @param payload_size Size of the payload
 */
void sendSerialMessage(const uint8_t* payload, uint16_t payload_size) {
    // Write magic byte
    Serial.write(MAGIC_BYTE);
    
    // Write length (2 bytes, little-endian)
    Serial.write((uint8_t*)&payload_size, LENGTH_SIZE);
    
    // Write protobuf payload
    Serial.write(payload, payload_size);
    
    // Ensure all data is sent
    Serial.flush();
}

/**
 * @brief Receive a framed serial message
 * @param buffer Buffer to store the received payload
 * @param buffer_size Maximum size of the buffer
 * @return Size of received payload, or 0 if no valid frame received
 */
size_t receiveSerialMessage(uint8_t* buffer, size_t buffer_size) {
    if (Serial.available() < FRAME_HEADER_SIZE) {
        return 0;  // Not enough data for header
    }
    
    // Check magic byte
    uint8_t magic = Serial.peek();
    if (magic != MAGIC_BYTE) {
        // Not a valid frame, consume the byte and return
        Serial.read();
        return 0;
    }
    
    // Read magic byte
    Serial.read();  // Consume magic byte
    
    // Read length
    uint16_t payload_size = 0;
    size_t bytes_read = Serial.readBytes((uint8_t*)&payload_size, LENGTH_SIZE);
    if (bytes_read != LENGTH_SIZE) {
        return 0;  // Failed to read length
    }
    
    // Validate payload size
    if (payload_size == 0 || payload_size > buffer_size) {
        return 0;  // Invalid payload size
    }
    
    // Wait for payload data with timeout
    unsigned long timeout = millis() + 100;  // 100ms timeout
    while (Serial.available() < payload_size) {
        if (millis() > timeout) {
            return 0;  // Timeout waiting for data
        }
        delay(1);
    }
    
    // Read payload
    bytes_read = Serial.readBytes(buffer, payload_size);
    if (bytes_read != payload_size) {
        return 0;  // Failed to read complete payload
    }
    
    return payload_size;
}

/**
 * @brief Parse a serial message and create a MeshMessage
 * @param buffer Raw serial data buffer
 * @param buffer_size Size of data in buffer
 * @param msg Output MeshMessage
 * @return true if parsing successful
 */
bool parseSerialMessage(const uint8_t* buffer, size_t buffer_size, MeshMessage& msg) {
    if (buffer_size == 0 || buffer == nullptr) {
        return false;
    }
    
    // Deserialize protobuf to MeshMessage
    size_t deserialized = MessageSerializer::deserialize(buffer, buffer_size, msg);
    return deserialized > 0;
}

/**
 * @brief Process received serial data and dispatch to mesh network
 */
void processIncomingSerialData() {
    static uint8_t rx_buffer[MessageSerializer::max_size()];
    static size_t rx_buffer_pos = 0;
    static enum { WAIT_MAGIC, WAIT_LENGTH, WAIT_PAYLOAD } state = WAIT_MAGIC;
    static uint16_t expected_length = 0;
    
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        
        switch (state) {
            case WAIT_MAGIC:
                if (byte == MAGIC_BYTE) {
                    state = WAIT_LENGTH;
                    rx_buffer_pos = 0;
                }
                break;
                
            case WAIT_LENGTH:
                if (rx_buffer_pos < LENGTH_SIZE) {
                    ((uint8_t*)&expected_length)[rx_buffer_pos] = byte;
                    rx_buffer_pos++;
                    
                    if (rx_buffer_pos >= LENGTH_SIZE) {
                        if (expected_length > 0 && expected_length <= sizeof(rx_buffer)) {
                            state = WAIT_PAYLOAD;
                            rx_buffer_pos = 0;
                        } else {
                            // Invalid length, reset
                            state = WAIT_MAGIC;
                        }
                    }
                }
                break;
                
            case WAIT_PAYLOAD:
                rx_buffer[rx_buffer_pos++] = byte;
                
                if (rx_buffer_pos >= expected_length) {
                    // Complete frame received
                    MeshMessage msg;
                    if (parseSerialMessage(rx_buffer, expected_length, msg)) {
                        // Send to mesh network
                        uint8_t serialized[MessageSerializer::max_size()];
                        size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
                        
                        if (serialized_size > 0) {
                            EspMeshFlood::instance().send_message(serialized, serialized_size, msg.ttl);
                        }
                    }
                    
                    // Reset for next frame
                    state = WAIT_MAGIC;
                    rx_buffer_pos = 0;
                }
                break;
        }
    }
}

static MeshMessage create_demo_message(uint32_t counter) {
    MeshMessage msg;
    msg.message_id = counter;
    msg.sender_id = EspMeshFlood::instance().get_node_id();
    msg.timestamp = millis();
    msg.type = MessageType::USER_MESSAGE;
    msg.ttl = 5;
    msg.relay_count = 0;

    const char* text = "Hello from ESP32";
    size_t payload_size = std::strlen(text);
    msg.payload.resize(payload_size);
    std::memcpy(msg.payload.data(), text, payload_size);

    return msg;
}

/**
 * @brief Callback when a message is received from mesh network.
 * Forwards to serial with proper framing
 */
void on_mesh_message_received(const MeshMessage& msg, int32_t rssi) {
    messages_received++;

    uint8_t serialized[MessageSerializer::max_size()];
    size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
    if (serialized_size > 0) {
        sendSerialMessage(serialized, serialized_size);
    }
}

/**
 * @brief Initialize the mesh network
 */
void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!EspMeshFlood::instance().init(on_mesh_message_received)) {
        while (1) {
            delay(1000);
        }
    }
}

/**
 * @brief Main loop - send periodic messages and maintain the mesh
 */
void loop() {
    unsigned long now = millis();
    
    // Process incoming serial data
    processIncomingSerialData();
    
    // Periodic message sending
    if (now - last_send_time >= SEND_INTERVAL_MS) {
        last_send_time = now;
        messages_sent++;

        MeshMessage msg = create_demo_message(messages_sent);
        uint8_t serialized[MessageSerializer::max_size()];
        size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));

        if (serialized_size > 0 && EspMeshFlood::instance().send_message(serialized, serialized_size, 5)) {
            sendSerialMessage(serialized, serialized_size);
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