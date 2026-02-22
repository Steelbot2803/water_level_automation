#include "MotorController.h"

void MotorController::begin() {
  pinMode(Pins::BOREWELL_RELAY, OUTPUT);
  pinMode(Pins::SUMP_RELAY, OUTPUT);
  digitalWrite(Pins::BOREWELL_RELAY, LOW);
  digitalWrite(Pins::SUMP_RELAY, LOW);
}

bool MotorController::start(MotorType motor, const SensorSnapshot& sensors, bool ignoreTankLevelLimit) {
  if (motor == MotorType::NONE) {
    return false;
  }
  if (activeMotor_ != MotorType::NONE && activeMotor_ != motor) {
    stop(activeMotor_);
  }
  if (!canStart(motor, sensors, ignoreTankLevelLimit)) {
    return false;
  }

  digitalWrite(relayPin(motor), HIGH);
  MotorState& state = stateRef(motor);
  state.status = MotorStatus::RUNNING;
  state.startedAtMs = millis();
  activeMotor_ = motor;
  return true;
}

void MotorController::stop(MotorType motor) {
  if (motor == MotorType::NONE) {
    return;
  }
  digitalWrite(relayPin(motor), LOW);
  stateRef(motor).status = MotorStatus::STOPPED;
  if (activeMotor_ == motor) {
    activeMotor_ = MotorType::NONE;
  }
}

void MotorController::stopAll() {
  stop(MotorType::BOREWELL);
  stop(MotorType::SUMP);
}

MotorState MotorController::stateOf(MotorType motor) const {
  switch (motor) {
    case MotorType::BOREWELL:
      return borewell_;
    case MotorType::SUMP:
      return sump_;
    case MotorType::NONE:
    default:
      return MotorState{};
  }
}

bool MotorController::canStart(MotorType motor, const SensorSnapshot& sensors, bool ignoreTankLevelLimit) const {
  if (dryRunTriggered(motor, sensors)) {
    return false;
  }
  if (motor == MotorType::SUMP && !ignoreTankLevelLimit) {
    return sensors.sumpLevel != SumpLevel::BELOW_CRITICAL;
  }
  return true;
}

MotorState& MotorController::stateRef(MotorType motor) {
  return motor == MotorType::BOREWELL ? borewell_ : sump_;
}

uint8_t MotorController::relayPin(MotorType motor) const {
  return motor == MotorType::BOREWELL ? Pins::BOREWELL_RELAY : Pins::SUMP_RELAY;
}

bool MotorController::dryRunTriggered(MotorType motor, const SensorSnapshot& sensors) const {
  return motor == MotorType::BOREWELL ? sensors.borewellDryRun : sensors.sumpDryRun;
}
