#ifdef ESP32

#include "espMeshFlood/transport/esp_now_transport_impl.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_idf_version.h>
#include <cstring>

namespace espMeshFlood {

static constexpr uint8_t kEspNowChannel = 1;
static constexpr int32_t kDefaultRssi = -50;

static uint8_t default_wifi_protocol_bitmap() {
    uint8_t bitmap = 0;
#ifdef WIFI_PROTOCOL_11B
    bitmap |= WIFI_PROTOCOL_11B;
#endif
#ifdef WIFI_PROTOCOL_11G
    bitmap |= WIFI_PROTOCOL_11G;
#endif
#ifdef WIFI_PROTOCOL_11N
    bitmap |= WIFI_PROTOCOL_11N;
#endif
    return bitmap;
}

// Static instance pointer for callback handling
static EspNowTransportImpl* g_transport_instance = nullptr;

/**
 * @brief Static callback wrapper for ESP-NOW receive
 * 
 * ESP-NOW callbacks are C-style, so we use a static function
 * that delegates to the instance.
 */
#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
static void esp_now_recv_cb_static(const esp_now_recv_info_t* recv_info, const uint8_t* data,
                                   int data_len) {
#else
static void esp_now_recv_cb_static(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
#endif
    if (g_transport_instance && data && data_len > 0) {
#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
        int32_t rssi = kDefaultRssi;
        if (recv_info && recv_info->rx_ctrl) {
            rssi = recv_info->rx_ctrl->rssi;
        }
#else
        (void)mac_addr;
        int32_t rssi = kDefaultRssi;
#endif
        
        g_transport_instance->on_data_received(data, static_cast<size_t>(data_len), rssi);
    }
}

EspNowTransportImpl::EspNowTransportImpl()
    : initialized_(false), node_id_(0), receive_callback_(nullptr) {
}

EspNowTransportImpl::~EspNowTransportImpl() {
    deinit();
}

uint32_t EspNowTransportImpl::extract_node_id_from_mac() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // Extract last 4 bytes of MAC as node ID (little-endian)
    uint32_t id = 0;
    id |= (static_cast<uint32_t>(mac[2]) << 24);
    id |= (static_cast<uint32_t>(mac[3]) << 16);
    id |= (static_cast<uint32_t>(mac[4]) << 8);
    id |= (static_cast<uint32_t>(mac[5]));
    
    return id;
}

bool EspNowTransportImpl::init() {
    if (initialized_) {
        return true;  // Already initialized
    }

    // Ensure WiFi is in STA mode (required for ESP-NOW)
    if (!WiFi.mode(WIFI_STA)) {
        Serial.println("[EspNowTransport] Failed to set WiFi STA mode");
        return false;
    }

    if (esp_wifi_set_channel(kEspNowChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        Serial.println("[EspNowTransport] Failed to set WiFi channel");
        return false;
    }

    uint8_t protocol_bitmap = 0;
    if (esp_wifi_get_protocol(WIFI_IF_STA, &protocol_bitmap) != ESP_OK) {
        protocol_bitmap = default_wifi_protocol_bitmap();
    }

    protocol_bitmap |= WIFI_PROTOCOL_LR;
    if (esp_wifi_set_protocol(WIFI_IF_STA, protocol_bitmap) != ESP_OK) {
        Serial.println("[EspNowTransport] Failed to enable WiFi long range protocol");
        return false;
    }
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[EspNowTransport] Failed to initialize ESP-NOW");
        return false;
    }

    // Register receive callback
    if (esp_now_register_recv_cb(esp_now_recv_cb_static) != ESP_OK) {
        Serial.println("[EspNowTransport] Failed to register receive callback");
        esp_now_deinit();
        return false;
    }

    // Add broadcast peer (FF:FF:FF:FF:FF:FF)
    esp_now_peer_info_t peer_info = {};
    std::memset(&peer_info, 0, sizeof(peer_info));
    std::memset(peer_info.peer_addr, 0xFF, ESP_NOW_ETH_ALEN);
    peer_info.channel = kEspNowChannel;
    peer_info.encrypt = false;

    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        Serial.println("[EspNowTransport] Failed to add broadcast peer");
        esp_now_deinit();
        return false;
    }

    // Extract and store node ID
    node_id_ = extract_node_id_from_mac();

    // Set instance pointer for static callback
    g_transport_instance = this;

    initialized_ = true;
    
    Serial.print("[EspNowTransport] Initialized with node ID: 0x");
    Serial.println(node_id_, HEX);
    Serial.println("[EspNowTransport] WiFi long range protocol enabled");

    return true;
}

void EspNowTransportImpl::deinit() {
    if (!initialized_) {
        return;
    }

    g_transport_instance = nullptr;
    esp_now_deinit();
    initialized_ = false;

    Serial.println("[EspNowTransport] Deinitialized");
}

bool EspNowTransportImpl::send_broadcast(const uint8_t* data, size_t length) {
    if (!initialized_ || !data || length == 0) {
        return false;
    }

    // Broadcast address
    uint8_t broadcast_addr[ESP_NOW_ETH_ALEN];
    std::memset(broadcast_addr, 0xFF, ESP_NOW_ETH_ALEN);

    // Send data
    esp_err_t result = esp_now_send(broadcast_addr, const_cast<uint8_t*>(data), length);
    
    return result == ESP_OK;
}

void EspNowTransportImpl::register_receive_callback(ReceiveCallback callback) {
    receive_callback_ = callback;
}

uint32_t EspNowTransportImpl::get_node_id() const {
    return node_id_;
}

bool EspNowTransportImpl::is_initialized() const {
    return initialized_;
}

/**
 * @brief Internal method called by static callback
 * 
 * This is called from the ESP-NOW receive callback and
 * delegates to the registered application callback.
 */
void EspNowTransportImpl::on_data_received(const uint8_t* data, size_t length, int32_t rssi) {
    if (receive_callback_) {
        receive_callback_(data, length, rssi);
    }
}

}  // namespace espMeshFlood

#endif  // ESP32
