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
unsigned long lastSuccessfulPublishMs = 0;
uint8_t lastWifiStatus = WL_NO_MODULE;
bool lastMqttConnected = false;

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
const char* wifiStatusCStr(uint8_t status) {
  switch (status) {
    case WL_CONNECTED: return "connected";
    case WL_IDLE_STATUS: return "connecting";
    case WL_NO_SSID_AVAIL: return "ssid_unavailable";
    case WL_CONNECT_FAILED: return "connect_failed";
    case WL_CONNECTION_LOST: return "connection_lost";
    case WL_DISCONNECTED: return "disconnected";
    case WL_NO_MODULE: return "no_module";
    default: return "unknown";
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
  if (lastWifiAttemptMs != 0 && nowMs - lastWifiAttemptMs < WIFI_RETRY_PERIOD_MS) return;

  lastWifiAttemptMs = nowMs;
  Serial.println(F("wifi: attempting connection"));
  WiFi.disconnect();
  WiFi.end();
  delay(200);
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
  if (lastMqttAttemptMs != 0 && nowMs - lastMqttAttemptMs < MQTT_RETRY_PERIOD_MS) return;

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

const char* manualTargetText(const SystemState& state) {
  if (!state.command.manualMode) return "none";
  return toStr(state.command.forcedMotor);
}

}

void initMqttLink() {
  mqttClient.setBufferSize(768);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  WiFi.disconnect();
  connectWifi();
  ensureMqttConnected();
}

void runMqttLink() {
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

  char payload[512];
  const int written = snprintf(
    payload,
    sizeof(payload),
    "{\"mode\":\"%s\",\"override\":%s,\"manual_target\":\"%s\","
    "\"overhead\":\"%s\",\"sump\":\"%s\",\"motor\":\"%s\","
    "\"borewell_status\":\"%s\",\"sump_status\":\"%s\","
    "\"sump_warning\":%s,\"emergency_stop\":%s,\"auto_prefer_sump\":%s,"
    "\"wifi_status\":\"%s\",\"mqtt_connected\":%s}",
    modeText(state),
    state.command.overrideFillToHigh ? "true" : "false",
    manualTargetText(state),
    toStr(state.overheadLevel),
    toStr(state.sumpLevel),
    toStr(state.activeMotor),
    toStr(state.borewell.status),
    toStr(state.sump.status),
    (state.sumpLevel == SumpLevel::CRITICAL || state.sumpCriticalWarningLatched) ? "true" : "false",
    state.command.emergencyStop ? "true" : "false",
    state.command.autoPreferSump ? "true" : "false",
    wifiStatusCStr(lastWifiStatus),
    mqttClient.connected() ? "true" : "false");

  if (written < 0 || written >= static_cast<int>(sizeof(payload))) {
    Serial.print(F("mqtt: payload truncated (needed "));
    Serial.print(written);
    Serial.println(F(" bytes), skipping publish"));
    return;
  }

  if (mqttClient.publish(MQTT_STATUS_TOPIC, payload, true)) {
    lastSuccessfulPublishMs = millis();
  } else {
    Serial.println(F("mqtt: publish failed"));
  }
}

void checkMqttLiveness() {
  if (lastSuccessfulPublishMs == 0) return;  // never published yet, still starting up
  const unsigned long gap = millis() - lastSuccessfulPublishMs;
  if (gap > MQTT_LIVENESS_TIMEOUT_MS) {
    Serial.println(F("mqtt: no successful publish in 10 min — forcing reset"));
    delay(100);  // let Serial flush
    NVIC_SystemReset();
  }
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isMqttConnected() {
  return mqttClient.connected();
}