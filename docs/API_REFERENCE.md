# EFMP API Reference

**Version**: 1.0  
**Date**: 2026-07-04

---

## Table of Contents

1. [EspMeshFlood (Public API)](#espmeshshflood-public-api)
2. [MeshProtocol](#meshprotocol)
3. [MessageCache](#messagecache)
4. [MessageSerializer](#messageserializer)
5. [RandomBackoff](#randombackoff)
6. [EspNowTransport](#espnowtransport)
7. [Types](#types)

---

## EspMeshFlood (Public API)

Singleton class providing the main interface for EFMP.

**Header**: `espMeshFlood/efmp.h`  
**Namespace**: `espMeshFlood`

### Constructor

```cpp
EspMeshFlood();
```

Private constructor (singleton pattern). Access via `instance()`.

### Static Methods

#### `static EspMeshFlood& instance()`

Returns the singleton instance.

```cpp
EspMeshFlood& efmp = EspMeshFlood::instance();
```

---

### Public Methods

#### `bool init(ApplicationCallback callback, std::shared_ptr<EspNowTransport> transport = nullptr)`

Initialize the EFMP library.

**Parameters**:
- `callback`: Function called when a message is received
  - Signature: `void(const MeshMessage& msg, int32_t rssi)`
  - `rssi`: Signal strength in dBm (0 for own messages)
- `transport` (optional): Custom transport implementation
  - If nullptr, uses default mock transport (desktop testing)
  - On ESP32, should use real EspNowTransportImpl

**Returns**: `true` if initialization succeeded, `false` otherwise

**Example**:
```cpp
void on_message(const MeshMessage& msg, int32_t rssi) {
    printf("Msg from 0x%X, rssi=%d\n", msg.sender_id, rssi);
}

EspMeshFlood::instance().init(on_message);
```

---

#### `void deinit()`

Deinitialize and cleanup resources.

```cpp
EspMeshFlood::instance().deinit();
```

---

#### `bool send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl = 5)`

Send a message into the mesh network.

**Parameters**:
- `payload`: Pointer to message data
- `payload_size`: Size in bytes (max 200 bytes)
- `ttl`: Time-to-live in hops (default: 5, max: 255)

**Returns**: `true` if message queued successfully, `false` if failed

**Behavior**:
- Creates message with auto-generated message_id
- Registers in cache to prevent loopback
- Broadcasts immediately via transport
- Delivers to application callback
- Application receives message with rssi=0 (indicates own message)

**Example**:
```cpp
uint8_t payload[] = {0xAA, 0xBB, 0xCC};
if (EspMeshFlood::instance().send_message(payload, sizeof(payload), 5)) {
    printf("Message sent\n");
} else {
    printf("Send failed\n");
}
```

---

#### `uint32_t get_node_id() const`

Get the local node's ID.

**Returns**: Node ID (typically MAC address last 4 bytes as uint32)

**Example**:
```cpp
uint32_t my_id = EspMeshFlood::instance().get_node_id();
printf("Node ID: 0x%08X\n", my_id);
```

---

#### `bool is_initialized() const`

Check if library is initialized.

**Returns**: `true` if init() was called and succeeded, `false` otherwise

---

#### `void do_maintenance()`

Perform periodic maintenance tasks.

**Tasks Performed**:
- Cleanup expired cache entries
- Check for pending retransmissions

**Call Frequency**: Every 1000 ms recommended

**Example**:
```cpp
void loop() {
    EspMeshFlood::instance().do_maintenance();
    delay(1000);
}
```

---

#### `void update_time(uint64_t current_time_ms)`

Update system time for cache management.

**Parameters**:
- `current_time_ms`: Current time in milliseconds (typically from millis() on Arduino)

**Notes**:
- Called automatically on ESP32 via system timer
- On desktop testing, update explicitly if testing time-dependent behavior

**Example**:
```cpp
uint64_t current_ms = millis();
EspMeshFlood::instance().update_time(current_ms);
```

---

## MeshProtocol

Core protocol implementation (internal).

**Header**: `espMeshFlood/protocol/mesh_protocol.h`  
**Namespace**: `espMeshFlood`

### Constructor

```cpp
MeshProtocol(std::shared_ptr<EspNowTransport> transport, 
             ApplicationCallback app_callback);
```

**Parameters**:
- `transport`: Transport layer implementation
- `app_callback`: Function to call on message received

---

### Public Methods

#### `bool init()`

Initialize protocol and transport layer.

**Returns**: `true` if successful

---

#### `void deinit()`

Shutdown protocol and transport.

---

#### `bool send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl = TTL_INITIAL)`

Send a message.

**Parameters**:
- `payload`: Message data
- `payload_size`: Payload size in bytes
- `ttl`: Time-to-live (hops)

**Returns**: `true` if sent successfully

**Internal Flow**:
1. Create MeshMessage with auto-generated message_id
2. Register in cache
3. Serialize message
4. Broadcast via transport
5. Deliver to application

---

#### `void on_message_received(const uint8_t* data, size_t length, int32_t rssi)`

Called by transport when a message is received.

**Parameters**:
- `data`: Serialized message bytes
- `length`: Message length in bytes
- `rssi`: Received signal strength

**Internal Flow**:
1. Deserialize message
2. Check if duplicate (cache)
3. If new: register in cache
4. Deliver to application
5. Schedule retransmission if TTL > 0

---

#### `uint32_t get_node_id() const`

Get local node ID.

---

#### `void update_time(uint64_t current_time_ms)`

Update current time for cache management.

---

#### `void do_maintenance()`

Perform periodic cleanup (cache expiration).

---

## MessageCache

Deduplication cache for tracking seen messages.

**Header**: `espMeshFlood/cache/message_cache.h`  
**Namespace**: `espMeshFlood`

### Constructor

```cpp
MessageCache(size_t max_size, uint64_t expiration_ms);
```

**Parameters**:
- `max_size`: Maximum number of entries (typically 100)
- `expiration_ms`: Entry lifetime in milliseconds (typically 600000 = 10 min)

---

### Public Methods

#### `bool exists(uint32_t sender_id, uint32_t message_id) const`

Check if a sender/message pair is in cache (without expiration check).

**Parameters**:
- `sender_id`: Sender ID associated with the message
- `message_id`: Message ID to look for

**Returns**: `true` if found, `false` otherwise

**Note**: Does not perform cleanup. Use `exists(sender_id, message_id, current_time)` for production.

---

#### `bool exists(uint32_t sender_id, uint32_t message_id, uint64_t current_time_ms)`

Check if a sender/message pair is in cache (with expiration check).

**Parameters**:
- `sender_id`: Sender ID associated with the message
- `message_id`: Message ID to look for
- `current_time_ms`: Current time for expiration calculation

**Returns**: `true` if found and not expired, `false` otherwise

**Side Effect**: Calls `cleanup_expired()` to remove stale entries

---

#### `void add(uint32_t sender_id, uint32_t message_id, uint64_t current_time_ms)`

Add a sender/message pair to the cache.

**Parameters**:
- `sender_id`: Sender ID associated with the message
- `message_id`: Message ID to add
- `current_time_ms`: Current time for expiration tracking

**Behavior**:
- Checks for duplicates (won't add if already present)
- If cache is full: evicts oldest entry
- Records current timestamp for expiration

---

#### `void cleanup_expired(uint64_t current_time_ms)`

Remove expired entries from cache.

**Parameters**:
- `current_time_ms`: Current time for expiration calculation

**Behavior**:
- Removes entries where `(current_time_ms - entry.timestamp) > expiration_ms`

---

#### `size_t size() const`

Get current number of entries in cache.

**Returns**: Number of active cache entries

---

#### `void clear()`

Remove all entries from cache.

---

## MessageSerializer

Protobuf-compatible serialization/deserialization.

**Header**: `espMeshFlood/serialization/message_serializer.h`  
**Namespace**: `espMeshFlood`

### Static Methods

#### `static size_t serialize(const MeshMessage& message, uint8_t* buffer, size_t buffer_size)`

Serialize a message to bytes.

**Parameters**:
- `message`: Message to serialize
- `buffer`: Output buffer pointer
- `buffer_size`: Size of output buffer (max 250 bytes)

**Returns**: Number of bytes written, or 0 if buffer too small

**Wire Format**: Protobuf varint encoding

**Example**:
```cpp
MeshMessage msg = {/* ... */};
uint8_t buffer[250];
size_t size = MessageSerializer::serialize(msg, buffer, sizeof(buffer));
if (size > 0) {
    transport->send_broadcast(buffer, size);
}
```

---

#### `static bool deserialize(const uint8_t* buffer, size_t buffer_size, MeshMessage& message)`

Deserialize bytes to a message.

**Parameters**:
- `buffer`: Input buffer pointer
- `buffer_size`: Size of input data
- `message`: Output message (filled on success)

**Returns**: `true` if deserialization succeeded, `false` if buffer invalid

**Behavior**:
- Initializes message with defaults
- Rejects empty buffers (buffer_size == 0)
- Parses Protobuf fields in order
- Skips unknown fields

**Example**:
```cpp
MeshMessage msg;
if (MessageSerializer::deserialize(buffer, size, msg)) {
    printf("Message from 0x%X\n", msg.sender_id);
}
```

---

#### `static size_t max_size()`

Get maximum serialized message size.

**Returns**: 250 (bytes)

---

## RandomBackoff

Random delay generator for retransmission backoff.

**Header**: `espMeshFlood/backoff/random_backoff.h`  
**Namespace**: `espMeshFlood`

### Constructor

```cpp
RandomBackoff(uint32_t min_delay_ms, uint32_t max_delay_ms, uint32_t seed = 0);
```

**Parameters**:
- `min_delay_ms`: Minimum delay in milliseconds (typically 50)
- `max_delay_ms`: Maximum delay in milliseconds (typically 300)
- `seed`: LCG seed for deterministic testing (0 = use system random)

---

### Public Methods

#### `uint32_t get_delay()`

Get next random delay value.

**Returns**: Random delay in milliseconds, uniformly distributed in [min, max]

**Algorithm**: LCG (Linear Congruential Generator) if seeded, else std::rand()

**Example**:
```cpp
RandomBackoff backoff(50, 300);  // 50-300 ms
for (int i = 0; i < 10; i++) {
    uint32_t delay = backoff.get_delay();
    printf("Delay: %u ms\n", delay);
}
```

---

#### `void set_seed(uint32_t seed)`

Change the random seed.

**Parameters**:
- `seed`: New seed value (0 = system random)

**Behavior**:
- Allows resetting to deterministic sequence
- Useful for testing reproducibility

---

## EspNowTransport

Abstract transport layer interface.

**Header**: `espMeshFlood/transport/esp_now_transport.h`  
**Namespace**: `espMeshFlood`

### Callback Type

```cpp
using ReceiveCallback = std::function<void(const uint8_t* data, size_t length, int32_t rssi)>;
```

---

### Virtual Methods

#### `virtual bool init()`

Initialize transport layer.

**Returns**: `true` if successful

---

#### `virtual void deinit()`

Shutdown transport layer.

---

#### `virtual bool send_broadcast(const uint8_t* data, size_t length)`

Send a broadcast message.

**Parameters**:
- `data`: Message bytes to send
- `length`: Message size in bytes

**Returns**: `true` if message queued, `false` if failed

---

#### `virtual void register_receive_callback(ReceiveCallback callback)`

Register callback for received messages.

**Parameters**:
- `callback`: Function called when message received

---

#### `virtual bool is_initialized() const`

Check if transport is initialized.

**Returns**: `true` if init() succeeded

---

#### `virtual uint32_t get_node_id() const`

Get this node's ID.

**Returns**: Node ID (typically MAC address)

---

## Types

### MeshMessage

Core message structure.

**Header**: `espMeshFlood/types.h`

```cpp
struct MeshMessage {
    uint32_t message_id;           // Unique message identifier
    uint32_t sender_id;            // Originating node ID
    uint64_t timestamp;            // Creation time (ms)
    MessageType type;              // USER_MESSAGE or HEARTBEAT
    uint32_t ttl;                  // Time-to-live (hops)
    std::vector<uint8_t> payload;  // Application data

    MeshMessage();  // Default constructor
};
```

**Field Details**:

| Field | Type | Default | Range | Notes |
|-------|------|---------|-------|-------|
| message_id | uint32 | 0 | 0-max | Auto-generated on send |
| sender_id | uint32 | 0 | 0-max | Set by originator |
| timestamp | uint64 | 0 | 0-max | Creation time in ms |
| type | enum | USER_MESSAGE | 0-1 | 0=data, 1=heartbeat |
| ttl | uint32 | 5 | 0-255 | Decremented on relay |
| payload | vector | empty | 0-200 bytes | Application data |

---

### MessageType

Enumeration for message types.

```cpp
enum class MessageType : uint32_t {
    USER_MESSAGE = 0,
    HEARTBEAT = 1,
};
```

---

### ApplicationCallback

Type alias for message receive callback.

```cpp
using ApplicationCallback = std::function<void(const MeshMessage& message, int32_t rssi)>;
```

**Parameters**:
- `message`: Received message
- `rssi`: Signal strength in dBm (0 for own messages)

---

## Config

Configuration constants.

**Header**: `espMeshFlood/config.h`

```cpp
class Config {
public:
    // TTL
    static constexpr uint32_t TTL_INITIAL = 5;
    static constexpr uint32_t TTL_MIN = 0;
    static constexpr uint32_t TTL_MAX = 255;

    // Cache
    static constexpr size_t CACHE_SIZE = 100;
    static constexpr uint64_t CACHE_ENTRY_EXPIRATION_MS = 10 * 60 * 1000;

    // Backoff
    static constexpr uint32_t MIN_RELAY_DELAY_MS = 50;
    static constexpr uint32_t MAX_RELAY_DELAY_MS = 300;

    // ESP-NOW
    static constexpr size_t MAX_PAYLOAD_SIZE = 250;
    static constexpr size_t MAX_MESSAGE_PAYLOAD_SIZE = 200;

    // Retransmission
    static constexpr size_t MAX_BROADCAST_RETRIES = 5;
};
```

**Customization**:

Edit `src/espMeshFlood/config.h` to adjust:
- TTL defaults and limits
- Cache size and expiration time
- Backoff delay range
- Message size limits

---

## Complete Example

```cpp
#include <espMeshFlood/efmp.h>

using namespace espMeshFlood;

// Global message counter
int messages_received = 0;

void on_message_received(const MeshMessage& msg, int32_t rssi) {
    messages_received++;
    
    printf("[MESH] Message #%d:\n", messages_received);
    printf("  From: 0x%08X\n", msg.sender_id);
    printf("  TTL: %u\n", msg.ttl);
    printf("  RSSI: %d dBm\n", rssi);
    printf("  Payload size: %zu bytes\n", msg.payload.size());
}

void setup() {
    // Initialize EFMP
    if (!EspMeshFlood::instance().init(on_message_received)) {
        printf("Failed to initialize EFMP\n");
        return;
    }
    
    printf("EFMP initialized, Node ID: 0x%08X\n", 
           EspMeshFlood::instance().get_node_id());
}

void loop() {
    // Periodic maintenance
    static unsigned long last_maintenance = 0;
    if (millis() - last_maintenance > 1000) {
        last_maintenance = millis();
        EspMeshFlood::instance().do_maintenance();
    }

    // Send a message every 5 seconds
    static unsigned long last_send = 0;
    if (millis() - last_send > 5000) {
        last_send = millis();
        
        uint8_t payload[] = {0x01, 0x02, 0x03};
        if (EspMeshFlood::instance().send_message(payload, sizeof(payload), 5)) {
            printf("Message sent\n");
        } else {
            printf("Send failed\n");
        }
    }
}
```

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-04
