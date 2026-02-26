#include "mqtt_link.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiS3.h>

#include "config.h"
#include "io.h"
#include "secrets.h"

namespace {

WiFiSSLClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastMqttPublishMs = 0;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastMqttAttemptMs = 0;
uint8_t lastWifiStatus = WL_NO_MODULE;
bool lastMqttConnected = false;

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

const __FlashStringHelper* wifiStatusText(uint8_t status) {
  switch (status) {
    case WL_CONNECTED: return F("connected");
    case WL_IDLE_STATUS: return F("connecting");
    case WL_NO_SSID_AVAIL: return F("ssid_unavailable");
    case WL_CONNECT_FAILED: return F("connect_failed");
    case WL_CONNECTION_LOST: return F("connection_lost");
    case WL_DISCONNECTED: return F("disconnected");
    case WL_NO_MODULE: return F("no_module");
    default: return F("unknown");
  }
}

void reportWifiStatusChange(uint8_t status) {
  if (status == lastWifiStatus) return;

  lastWifiStatus = status;
  Serial.print(F("wifi_status: "));
  Serial.println(wifiStatusText(status));
}

void reportMqttStatusChange(bool connected) {
  if (connected == lastMqttConnected) return;

  lastMqttConnected = connected;
  Serial.print(F("mqtt_status: "));
  Serial.println(connected ? F("connected") : F("disconnected"));
}

void connectWifi() {
  const uint8_t status = WiFi.status();
  reportWifiStatusChange(status);

  if (status == WL_CONNECTED) return;

  const unsigned long nowMs = millis();
  if (lastWifiAttemptMs != 0 && nowMs - lastWifiAttemptMs < 30000) return;

  lastWifiAttemptMs = nowMs;
  Serial.println(F("wifi: attempting connection"));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
  if (WiFi.status() != WL_CONNECTED) {
    reportMqttStatusChange(false);
    return;
  }

  if (mqttClient.connected()) {
    reportMqttStatusChange(true);
    return;
  }

  const unsigned long nowMs = millis();
  if (lastMqttAttemptMs != 0 && nowMs - lastMqttAttemptMs < 5000) return;

  lastMqttAttemptMs = nowMs;
  Serial.println(F("mqtt: attempting broker connection"));
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttClient.subscribe(MQTT_COMMAND_TOPIC);
    reportMqttStatusChange(true);
    return;
  }

  reportMqttStatusChange(false);
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

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isMqttConnected() {
  return mqttClient.connected();
}