#include "config.h"
#include "controller.h"
#include "io.h"
#include "status.h"
#include "mqtt_link.h"
#include "secrets.h"
#include "watchdog.h"
#include "led_matrix.h"
#include "persistence.h"

SystemState state;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.flush();
  initPersistence();

  pinMode(PIN_BOREWELL_RELAY, OUTPUT);
  pinMode(PIN_SUMP_RELAY, OUTPUT);

  stopMotor(MotorType::BOREWELL);
  stopMotor(MotorType::SUMP);

  initState(state);
  setSystemStateRef(&state);
  printBanner();
  initLedMatrix();
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
  updateLedMatrix(state);
  publishStatus(state);
  publishStateToMqtt(state);
  checkMqttLiveness();
  delay(MAIN_LOOP_DELAY_MS);
}
