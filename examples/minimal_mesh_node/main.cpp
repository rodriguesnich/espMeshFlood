#include <Arduino.h>
#include <espMeshFlood/efmp.h>
#include <espMeshFlood/types.h>
#include <espMeshFlood/serialization/message_serializer.h>
#include <espMeshFlood/transport/esp_now_transport_impl.h>
#include <espMeshFlood/host/serial_host_transport.h>
#include <cstring>

using namespace espMeshFlood;

// Host transport — wraps Serial with the unified EFMP framing protocol.
// The template parameter makes this testable with MockStream without Arduino headers.
SerialHostTransport<HardwareSerial> host_transport(Serial, 115200);

// Global timing
unsigned long last_heartbeat_time = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
unsigned long last_maintenance_time = 0;
const unsigned long MAINTENANCE_INTERVAL_MS = 1000;

// Statistics
uint32_t messages_sent = 0;
uint32_t messages_received = 0;
uint32_t heartbeat_count = 0;

bool sendHeartbeat() {
    MeshMessage msg;
    msg.message_id = heartbeat_count + 1;
    msg.sender_id  = EspMeshFlood::instance().get_node_id();
    msg.timestamp  = millis();
    msg.type       = MessageType::HEARTBEAT;
    msg.ttl        = 3;
    msg.relay_count = 0;

    char heartbeat_payload[32];
    snprintf(heartbeat_payload, sizeof(heartbeat_payload), "HB:%lu", millis() / 1000);
    const size_t payload_size = std::strlen(heartbeat_payload);
    msg.payload.resize(payload_size);
    std::memcpy(msg.payload.data(), heartbeat_payload, payload_size);

    uint8_t serialized[MessageSerializer::max_size()];
    const size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
    if (serialized_size == 0) return false;

    const bool sent = EspMeshFlood::instance().send_message(serialized, serialized_size, msg.ttl);
    if (sent) {
        heartbeat_count++;
        messages_sent++;
        host_transport.send(serialized, serialized_size);
    }
    return sent;
}

void on_mesh_message_received(const MeshMessage& msg, int32_t /*rssi*/) {
    messages_received++;

    // Skip our own heartbeats echoing back from the mesh — already sent in sendHeartbeat().
    if (msg.type == MessageType::HEARTBEAT &&
        msg.sender_id == EspMeshFlood::instance().get_node_id()) {
        return;
    }

    uint8_t serialized[MessageSerializer::max_size()];
    const size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
    if (serialized_size > 0) {
        host_transport.send(serialized, serialized_size);
    }
}

void setup() {
    // init() calls Serial.begin(115200) internally
    host_transport.init();
    delay(1000);

    Serial.println("ESP32 Mesh Node (Serial) starting...");

    auto transport = std::make_shared<espMeshFlood::EspNowTransportImpl>();
    if (!EspMeshFlood::instance().init(on_mesh_message_received, transport)) {
        Serial.println("Failed to initialize mesh!");
        while (1) delay(1000);
    }

    // Register host → mesh receive callback.
    // FrameParser strips framing; callback receives raw protobuf bytes.
    host_transport.register_receive_callback([](const uint8_t* data, size_t len) {
        MeshMessage msg;
        if (MessageSerializer::deserialize(data, len, msg)) {
            EspMeshFlood::instance().send_message(data, len, msg.ttl);
            messages_sent++;
        }
    });

    Serial.println("Mesh initialized successfully");
    Serial.print("Node ID: ");
    Serial.println(EspMeshFlood::instance().get_node_id());
}

void loop() {
    const unsigned long now = millis();

    // Drain serial RX bytes into the frame parser (highest priority).
    host_transport.poll();

    if (now - last_heartbeat_time >= HEARTBEAT_INTERVAL_MS) {
        last_heartbeat_time = now;
        sendHeartbeat();
    }

    if (now - last_maintenance_time >= MAINTENANCE_INTERVAL_MS) {
        last_maintenance_time = now;
        EspMeshFlood::instance().update_time(now);
        EspMeshFlood::instance().do_maintenance();
    }

    delay(100);
}