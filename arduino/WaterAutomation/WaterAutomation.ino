#include "config.h"
#include "controller.h"
#include "io.h"
#include "status.h"
#include "mqtt_link.h"
#include "secrets.h"
#include "watchdog.h"

SystemState state;

void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_BOREWELL_RELAY, OUTPUT);
  pinMode(PIN_SUMP_RELAY, OUTPUT);
  pinMode(PIN_BOREWELL_FLOW_OK, INPUT_PULLUP);
  pinMode(PIN_SUMP_FLOW_OK, INPUT_PULLUP);

  stopMotor(MotorType::BOREWELL);
  stopMotor(MotorType::SUMP);

  initState(state);
  setSystemStateRef(&state);
  printBanner();
  initMqttLink();

  if (!watchdogBegin()) {
    Serial.println(F("WARNING: watchdog not active"));
  }
}

void loop() {

  watchdogKick();

  updateLevelsFromPins(state);
  readCommandFromSerial(state);
  runMqttLink();
  runAutomationLogic(state);
  writeMotorOutputs(state);
  publishStatus(state);
  publishStateToMqtt(state);
  delay(MAIN_LOOP_DELAY_MS);
}
