#pragma once

// Copy this file to `secrets.h` and fill with your own values.

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

constexpr char MQTT_HOST[] = "YOUR_HIVEMQ_HOST";      // e.g. xxxxx.s1.eu.hivemq.cloud
constexpr uint16_t MQTT_PORT = 1883;                  // use 8883 for TLS if supported by your board setup
constexpr char MQTT_CLIENT_ID[] = "water-level-arduino";
constexpr char MQTT_USERNAME[] = "YOUR_HIVEMQ_USERNAME";
constexpr char MQTT_PASSWORD[] = "YOUR_HIVEMQ_PASSWORD";

constexpr char MQTT_COMMAND_TOPIC[] = "water-system/cmd";
constexpr char MQTT_STATUS_TOPIC[] = "water-system/status";
