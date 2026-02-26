#include "mqtt_link.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "io.h"
#include "secrets.h"

namespace {

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastMqttPublishMs = 0;

const char* overheadText(OverheadLevel l) {
  switch (l) {
    case OverheadLevel::EMPTY: return "empty";
    case OverheadLevel::CRITICAL: return "critical";
    case OverheadLevel::LOW: return "low";
    case OverheadLevel::MEDIUM: return "medium";
    case OverheadLevel::HIGH: return "high";
  }
  return "unknown";
}

const char* sumpText(SumpLevel l) {
  switch (l) {
    case SumpLevel::BELOW_CRITICAL: return "below_critical";
    case SumpLevel::CRITICAL: return "critical";
    case SumpLevel::LOW: return "low";
    case SumpLevel::HIGH: return "high";
  }
  return "unknown";
}

const char* motorText(MotorType m) {
  switch (m) {
    case MotorType::NONE: return "none";
    case MotorType::BOREWELL: return "borewell";
    case MotorType::SUMP_TRANSFER: return "sump_transfer";
  }
  return "unknown";
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(250);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String cmd;
  cmd.reserve(length);
  for (unsigned int i = 0; i < length; ++i) {
    cmd += static_cast<char>(payload[i]);
  }

  cmd.trim();
  cmd.toLowerCase();

  SystemState* state = getSystemStateRef();
  if (state == nullptr || cmd.length() == 0) return;

  if (!applyCommand(*state, cmd)) {
    Serial.print(F("MQTT unknown command on topic "));
    Serial.print(topic);
    Serial.print(F(": "));
    Serial.println(cmd);
  }
}

void ensureMqttConnected() {
  if (mqttClient.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      mqttClient.subscribe(MQTT_COMMAND_TOPIC);
      return;
    }
    delay(1000);
  }
}

const char* modeText(const SystemState& state) {
  return state.command.manualMode ? "manual" : "auto";
}

}  // namespace

void initMqttLink() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  connectWifi();
  ensureMqttConnected();
}

void runMqttLink(SystemState& state) {
  (void)state;
  connectWifi();
  ensureMqttConnected();

  if (!mqttClient.connected()) return;
  mqttClient.loop();
}

void publishStateToMqtt(const SystemState& state) {
  if (!mqttClient.connected()) return;

  const unsigned long nowMs = millis();
  if (lastMqttPublishMs != 0 && nowMs - lastMqttPublishMs < MQTT_PUBLISH_PERIOD_MS) {
    return;
  }
  lastMqttPublishMs = nowMs;

  char payload[220];
  snprintf(
      payload,
      sizeof(payload),
      "{\"mode\":\"%s\",\"override\":%s,\"overhead\":\"%s\",\"sump\":\"%s\",\"motor\":\"%s\"}",
      modeText(state),
      state.command.overrideFillToHigh ? "true" : "false",
      overheadText(state.overheadLevel),
      sumpText(state.sumpLevel),
      motorText(state.activeMotor));

  mqttClient.publish(MQTT_STATUS_TOPIC, payload, true);
}
