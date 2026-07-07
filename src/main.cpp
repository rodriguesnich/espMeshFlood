// Build shim for PlatformIO: selects the correct example based on the active env.
//   pio run -e esp32_serial  →  Serial framing via minimal_mesh_node
//   pio run -e esp32_ble     →  BLE GATT   via bluetooth_mesh_node
#ifdef ESP32
  #ifdef HOST_TRANSPORT_BLE
    #include "../examples/bluetooth_mesh_node/main.cpp"
  #else
    #include "../examples/minimal_mesh_node/main.cpp"
  #endif
#endif