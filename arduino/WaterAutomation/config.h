#pragma once

#include <Arduino.h>

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr unsigned long MAIN_LOOP_DELAY_MS = 200;
constexpr unsigned long STATUS_PRINT_PERIOD_MS = 1000;
constexpr unsigned long DRY_RUN_GRACE_MS = 5000;
constexpr unsigned long MQTT_HEARTBEAT_PERIOD_MS = 5000;
constexpr unsigned long MQTT_PUBLISH_PERIOD_MS = 500;
constexpr unsigned long MQTT_LIVENESS_TIMEOUT_MS = 10UL * 60UL * 1000UL;
constexpr unsigned long MQTT_RETRY_PERIOD_MS = 5000;
constexpr unsigned long WIFI_RETRY_PERIOD_MS = 10000;
constexpr int MOTOR_CURRENT_THRESHOLD = 100;
constexpr unsigned long AC_PROBE_SETTLE_MS = 60;
constexpr float AC_PROBE_FREQ_HZ = 1000.0f;

// Relays (active HIGH assumed)
constexpr uint8_t PIN_BOREWELL_RELAY = A2;
constexpr uint8_t PIN_SUMP_RELAY = A3;

// Motor flow/feedback (Analog sensing)
constexpr uint8_t PIN_BOREWELL_CURRENT = A0;
constexpr uint8_t PIN_SUMP_CURRENT = A1;

// AC probe transmit pins
constexpr uint8_t PIN_OH_TX = 5;
constexpr uint8_t PIN_SUMP_TX = 6;

// Overhead tank level switches (HIGH = switch active)
constexpr uint8_t PIN_OH_LOW = 4;
constexpr uint8_t PIN_OH_MED = 7;
constexpr uint8_t PIN_OH_HIGH = 8;

// Sump tank level switches (HIGH = switch active)
constexpr uint8_t PIN_SUMP_LOW = 12;
constexpr uint8_t PIN_SUMP_HIGH = 13;