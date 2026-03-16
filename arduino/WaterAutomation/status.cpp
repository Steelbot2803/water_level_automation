#include "mqtt_link.h"
#include "status.h"

void publishStatus(SystemState& state) {
  const unsigned long nowMs = millis();
  if (state.lastStatusPrintMs != 0 && nowMs - state.lastStatusPrintMs < STATUS_PRINT_PERIOD_MS) {
    return;
  }
  state.lastStatusPrintMs = nowMs;

  Serial.println(F("--- water-system-status ---"));

  Serial.print(F("mode: "));
  Serial.println(state.command.manualMode ? F("manual") : F("auto"));

  Serial.print(F("override_fill_to_high: "));
  Serial.println(state.command.overrideFillToHigh ? F("true") : F("false"));

  Serial.print(F("emergency_stop: "));
  Serial.println(state.command.emergencyStop ? F("true") : F("false"));

  Serial.print(F("auto_prefer_sump: "));
  Serial.println(state.command.autoPreferSump ? F("true") : F("false"));

  Serial.print(F("overhead_level: "));
  Serial.println(toText(state.overheadLevel));

  Serial.print(F("sump_level: "));
  Serial.println(toText(state.sumpLevel));

  Serial.print(F("active_motor: "));
  Serial.println(toText(state.activeMotor));

  Serial.print(F("borewell_status: "));
  Serial.println(toText(state.borewell.status));

  Serial.print(F("sump_status: "));
  Serial.println(toText(state.sump.status));

  Serial.print(F("wifi_connected: "));
  Serial.println(isWifiConnected() ? F("true") : F("false"));

  Serial.print(F("mqtt_connected: "));
  Serial.println(isMqttConnected() ? F("true") : F("false"));

  if (state.sumpLevel == SumpLevel::CRITICAL || state.sumpCriticalWarningLatched) {
    Serial.println(F("warning: sump is CRITICAL - sump transfer motor blocked."));
  }
}