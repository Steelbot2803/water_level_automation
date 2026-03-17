#pragma once

#include <Arduino.h>

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr unsigned long MAIN_LOOP_DELAY_MS = 200;
constexpr unsigned long STATUS_PRINT_PERIOD_MS = 1000;
constexpr unsigned long DRY_RUN_GRACE_MS = 5000;
constexpr unsigned long MQTT_PUBLISH_PERIOD_MS = 2000;
constexpr unsigned long MQTT_RETRY_PERIOD_MS = 5000;   // between MQTT reconnect attempts
constexpr unsigned long WIFI_RETRY_PERIOD_MS = 10000;  // between WiFi reconnect attempts

// START_RETRY_LOCK_MS is no longer used — the dry-run lockout is now permanent
// (dryRunLatched flag) rather than time-based. Safe to delete if desired.
// constexpr unsigned long START_RETRY_LOCK_MS = 10000;

// Relays (active HIGH assumed)
constexpr uint8_t PIN_BOREWELL_RELAY = 4;
constexpr uint8_t PIN_SUMP_RELAY = 5;

// Motor flow/feedback (LOW = water/flow detected with pullup wiring)
constexpr uint8_t PIN_BOREWELL_FLOW_OK = 6;
constexpr uint8_t PIN_SUMP_FLOW_OK = 7;

// Overhead tank level switches (LOW = switch active)
constexpr uint8_t PIN_OH_LOW = 9;
constexpr uint8_t PIN_OH_MED = 10;
constexpr uint8_t PIN_OH_HIGH = 11;

// Sump tank level switches (LOW = switch active)
constexpr uint8_t PIN_SUMP_LOW = 12;
constexpr uint8_t PIN_SUMP_HIGH = 13;