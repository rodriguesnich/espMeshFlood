# ESP Flood Mesh Protocol (EFMP) - Specification

**Version**: 1.0 MVP  
**Date**: 2026-07-04  
**Status**: Complete and Validated (43/43 tests passing)

---

## Table of Contents

1. [Overview](#overview)
2. [Protocol Requirements](#protocol-requirements)
3. [Message Format](#message-format)
4. [Node Behavior](#node-behavior)
5. [Deduplication](#deduplication)
6. [TTL Management](#ttl-management)
7. [Retransmission](#retransmission)
8. [Configuration](#configuration)
9. [Test Cases](#test-cases)
10. [Performance](#performance)

---

## Overview

EFMP is a lightweight broadcast flooding protocol for ESP32 mesh networks using ESP-NOW. It enables efficient message dissemination without requiring route discovery or centralized routing infrastructure.

**Protocol Type**: Flood Gossip  
**Transport**: ESP-NOW (IEEE 802.11 ad-hoc)  
**Scope**: Local network mesh (WiFi STA range)  
**Message Delivery**: Best-effort, no ACK  

### Design Goals

- ✅ Simple and efficient message flooding
- ✅ Automatic duplicate detection
- ✅ Configurable propagation distance (TTL)
- ✅ Collision mitigation through random delays
- ✅ Minimal overhead and memory footprint

---

## Protocol Requirements

### R1: Broadcast Flooding
Every node that receives a new message MUST retransmit it to all its neighbors (broadcast address).

**Rationale**: Ensures maximum coverage without requiring topology knowledge.

### R2: Automatic Deduplication
Nodes MUST maintain a cache of recently seen message IDs and discard duplicates.

**Rationale**: Prevents infinite loops and reduces redundant processing.

### R3: Hop Limit (TTL)
Every message carries a TTL field. Nodes MUST decrement TTL before retransmission and MUST NOT retransmit if TTL reaches 0.

**Rationale**: Limits propagation distance and prevents network saturation.

### R4: Delivery Metrics (Reserved for v2)
Sender-side delivery tracking and receipt acknowledgments are reserved for future versions. MVP focuses on core TTL-based propagation control.

**Rationale**: Broadcast flooding without ACK mechanisms cannot reliably determine receiver count. Future v2 versions may add optional receipt messages to provide sender feedback on message delivery.

### R5: Random Retransmission Delay
Nodes MUST wait a random delay before retransmitting to reduce collision probability.

**Rationale**: Mitigates synchronized retransmission storms from multi-hop scenarios.

### R6: Deterministic Ordering
Message processing order: Deserialize → Dedup check → Register cache → Deliver app → Schedule retransmit.

**Rationale**: Prevents race conditions and ensures consistency.

---

## Message Format

### Wire Format: Protobuf (nanopb-compatible)

```protobuf
message MeshMessage {
   uint32 message_id = 1;      // Unique identifier (local sender counter)
    uint32 sender_id = 2;       // Originator node ID (MAC-derived)
    uint64 timestamp = 3;       // Creation time (milliseconds)
    MessageType type = 4;       // USER_MESSAGE=0, HEARTBEAT=1
    uint32 ttl = 5;             // Time to live (hops remaining)
    bytes payload = 6;          // Application data
}

enum MessageType {
    USER_MESSAGE = 0;
    HEARTBEAT = 1;
}
```

### Field Specifications

| Field | Type | Size | Description |
|-------|------|------|-------------|
| message_id | uint32 | 4 | Unique per sender process; generated from local counter |
| sender_id | uint32 | 4 | MAC address last 4 bytes as uint32 |
| timestamp | uint64 | 8 | ms since node boot or epoch |
| type | enum | 1 | 0=user data, 1=heartbeat |
| ttl | uint32 | 4 | Initial value: 5; min: 0, max: 255 |
| payload | bytes | 0-200 | Application data |

### Serialization Rules

- **Wire Type 0 (Varint)**: message_id, sender_id, timestamp, type, ttl
- **Wire Type 2 (Length-delimited)**: payload
- **Byte Order**: Little-endian varint encoding (Protobuf standard)
- **Maximum Size**: 250 bytes (ESP-NOW limit); payload typically 150-180 bytes

---

## Node Behavior

### Originator Node (Sender)

```
1. Create MeshMessage
   - message_id = generate_id()
   - sender_id = node_id
   - timestamp = current_time
   - type = USER_MESSAGE
   - ttl = TTL_INITIAL
   - payload = application_data

2. Register in dedup cache
   - cache.add(message_id, current_time)

3. Serialize message
   - buffer = serialize(message)

4. Broadcast via ESP-NOW
   - send_broadcast(buffer)

5. Deliver to local application
   - app_callback(message, rssi=0)
```

### Receiver Node (Relay)

```
1. Receive from transport layer
   - message_bytes, rssi = esp_now_receive()

2. Deserialize
   - message = deserialize(message_bytes)
   - if failed: discard

3. Check deduplication cache
   - if cache.exists(message_id):
       discard (duplicate)

4. Register in dedup cache
   - cache.add(message_id, current_time)

5. Deliver to local application
   - app_callback(message, rssi)

6. Schedule retransmission
   - if message.ttl > 0:
       schedule_retransmit(message)
```

### Retransmission

```
1. Wait random delay
   - delay_ms = random(MIN_RELAY_DELAY, MAX_RELAY_DELAY)
   - sleep(delay_ms)

2. Create retransmission copy
   - retx_msg = message
   - retx_msg.ttl = message.ttl - 1

3. Broadcast
   - buffer = serialize(retx_msg)
   - send_broadcast(buffer)

4. Note: TTL can be 0 when retransmitting
   - Allows one final hop at TTL=0 boundary
```

---

## Deduplication

### Cache Structure

```cpp
struct CacheEntry {
   uint32 message_id;
    uint64 timestamp_ms;  // when added
};

class MessageCache {
    vector<CacheEntry> entries;  // max_size = 100
    uint64 expiration_ms = 600000; // 10 minutes
};
```

### Deduplication Logic

1. **Check Phase**: `exists(msg_id, current_time)`
   - Cleanup expired entries
   - Linear search for message_id
   - Return true if found

2. **Add Phase**: `add(msg_id, current_time)`
   - Check if already exists (avoid duplicates)
   - If cache full: evict oldest entry
   - Add new entry with current timestamp

3. **Expiration**: `cleanup_expired(current_time)`
   - Remove entries where `(current_time - entry.timestamp) > expiration_ms`
   - Called on every `exists()` check
   - Also called periodically during maintenance

### Properties

- **Prevents Loops**: Duplicate messages are rejected, breaking broadcast loops
- **Bounded Memory**: Cache size limited to 100 entries
- **Automatic Cleanup**: Entries expire after 10 minutes
- **FIFO Eviction**: Oldest entries removed when cache fills

---

## TTL Management

### TTL Semantics

- **Initial Value**: 5 hops (configurable)
- **Decrement**: TTL is decremented by 1 on each retransmission
- **Boundary**: Message with TTL=0 is delivered but NOT retransmitted
- **Minimum**: 0 (no further propagation)

### Propagation Distance Examples

| TTL | Max Hops | Example Topology |
|-----|----------|-----------------|
| 1 | 1 | Direct link: A → B only |
| 2 | 2 | Linear chain: A → B → C |
| 3 | 3 | Linear chain: A → B → C → D |
| 5 | 5 | Network with depth 5 |

### TTL Logic in Retransmission

```
if message.ttl > 0:
    retx_msg.ttl = message.ttl - 1
    broadcast(retx_msg)
else:
    # TTL is 0, do not retransmit
    return
```

**Note**: The retransmitted message may have TTL=0, which is valid. It will be delivered to local app but not retransmitted further.

---

## Retransmission

### Random Backoff

**Purpose**: Reduce collision probability in multi-relay scenarios

**Implementation**: Linear Congruential Generator (LCG) for deterministic testing

```cpp
delay_ms = random(MIN_RELAY_DELAY_MS, MAX_RELAY_DELAY_MS)
// delay_ms ∈ [50, 300] milliseconds
```

**Algorithm (LCG)**:
```
state = (1103515245 * state + 12345) & 0x7FFFFFFF
offset = state % (max - min + 1)
delay = min + offset
```

### Timing

- **Arduino/ESP32**: `delay(delay_ms)`
- **Native/Desktop**: `std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms))`

### Rationale

Without random delays, multiple nodes receiving the same message would retransmit simultaneously, causing collisions. Random delays spread retransmissions over time.

---

## Configuration

### Compile-Time Constants

File: `src/espMeshFlood/config.h`

```cpp
class Config {
    // TTL
    static constexpr uint32_t TTL_INITIAL = 5;
    static constexpr uint32_t TTL_MIN = 0;
    static constexpr uint32_t TTL_MAX = 255;

    // Cache
    static constexpr size_t CACHE_SIZE = 100;
    static constexpr uint64_t CACHE_ENTRY_EXPIRATION_MS = 10 * 60 * 1000;  // 10 min

    // Backoff
    static constexpr uint32_t MIN_RELAY_DELAY_MS = 50;
    static constexpr uint32_t MAX_RELAY_DELAY_MS = 300;

    // ESP-NOW
    static constexpr size_t MAX_PAYLOAD_SIZE = 250;
    static constexpr size_t MAX_MESSAGE_PAYLOAD_SIZE = 200;
};
```

### Runtime Configuration

```cpp
// Override via constructor parameters or environment
EspMeshFlood::instance().init(
    callback,
    custom_transport,  // optional
    custom_ttl         // optional
);
```

---

## Performance

### Memory Footprint

| Component | Size | Notes |
|-----------|------|-------|
| Library Code | ~20 KB | ESP32 flash (library only) |
| Cache | ~2 KB | 100 entries × 16 bytes/entry |
| Message Buffer | ~250 bytes | Single message serialization |
| Backoff State | ~8 bytes | LCG state (uint32 + flags) |

**Total**: ~22 KB + buffers (minimal for ESP32)

### Message Overhead

| Component | Size |
|-----------|------|
| message_id | up to 5 bytes (varint encoded) |
| sender_id | 4 bytes |
| timestamp | 8 bytes |
| type | 1 byte |
| ttl | 1 byte |
| **Overhead** | **~26 bytes** |
| **Max Payload** | **~224 bytes** |

### Timing

| Operation | Latency |
|-----------|---------|
| Serialize | < 1 ms |
| Deserialize | < 1 ms |
| Cache lookup | < 1 ms (100 entries) |
| Random delay | 50-300 ms (configurable) |
| **Per-hop latency** | **~100-500 ms** |

### Throughput

- **Messages/sec (single node)**: Limited by backoff delay (50-300 ms)
- **Typical rate**: 3-20 msg/sec depending on TTL and topology

---

## Limitations (MVP)

### Scope Exclusions

- ❌ No unicast messaging (broadcast only)
- ❌ No encryption or authentication
- ❌ No clock synchronization
- ❌ No message fragmentation
- ❌ No adaptive TTL
- ❌ No priority/QoS

### ESP-NOW Constraints

- Max packet: 250 bytes
- No ACK on broadcast
- Range: ~100-250m (environment-dependent)
- Single channel (configured externally)

### Network Assumptions

- Relatively small networks (< 100 nodes)
- Stable topology (not mobile swarms)
- Sufficient bandwidth for flooding

---

## Future Enhancements (v2+)

- [ ] **Unicast Messaging**: Direct node-to-node with ACK
- [ ] **Encryption**: AES-128 message encryption
- [ ] **Selective Flooding**: Probability-based transmission
- [ ] **Adaptive TTL**: Dynamic hop limit based on network density
- [ ] **Priority Queues**: Message importance levels
- [ ] **Statistics**: Per-node delivery metrics

---

## References

- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Protobuf Encoding](https://developers.google.com/protocol-buffers/docs/encoding)
- [Gossip Algorithms](https://en.wikipedia.org/wiki/Gossip_protocol)
- [nanopb](https://github.com/nanopb/nanopb)

---

## Appendix: Test Results Summary

```
Total Tests: 43
Passed: 43 ✅
Failed: 0

Breakdown:
  - Cache Tests: 8 ✅
  - Metadata Tests: 13 ✅
  - Backoff Tests: 8 ✅
  - Serialization Tests: 8 ✅
  - Integration Tests: 6 ✅
```

**Build Status**:
- ESP32: ✅ SUCCESS
- Native (Desktop): ✅ SUCCESS

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-04  
**Status**: Complete and Validated
