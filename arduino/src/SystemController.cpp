#include "SystemController.h"

#include "Config.h"

void SystemController::begin() {
  sensors_.begin();
  motors_.begin();
  commands_.begin();
}

void SystemController::loop() {
  commands_.poll(command_);
  sensorSnapshot_ = sensors_.read();

  runLogic();
  enforceDryRunSafety();

  if (millis() - lastStatusAtMs_ >= Timing::STATUS_PRINT_INTERVAL_MS) {
    printStatus();
    lastStatusAtMs_ = millis();
  }
}

void SystemController::runLogic() {
  if (sensorSnapshot_.sumpLevel == SumpLevel::CRITICAL) {
    Serial.println(F("WARN: sump reached critical level"));
  }

  if (command_.mode == ControlMode::MANUAL) {
    if (command_.manualMotor == MotorType::NONE) {
      motors_.stopAll();
      return;
    }

    const bool ignoreTankLevelLimit = true;
    if (!motors_.start(command_.manualMotor, sensorSnapshot_, ignoreTankLevelLimit)) {
      motors_.stop(command_.manualMotor);
    }
    return;
  }

  if (sensorSnapshot_.sumpLevel == SumpLevel::BELOW_CRITICAL) {
    motors_.stop(MotorType::SUMP);
    if (motors_.activeMotor() == MotorType::SUMP) {
      motors_.stopAll();
    }
  }

  if (!shouldFillToHigh()) {
    motors_.stopAll();
    command_.forceStart = false;
    if (command_.mode == ControlMode::OVERRIDE_FILL) {
      command_.mode = ControlMode::AUTO;
    }
    return;
  }

  selectAndStartPreferredMotor(false);
}

void SystemController::enforceDryRunSafety() {
  if (sensorSnapshot_.borewellDryRun && motors_.activeMotor() == MotorType::BOREWELL) {
    Serial.println(F("FAULT: borewell dry run"));
    motors_.stop(MotorType::BOREWELL);
  }
  if (sensorSnapshot_.sumpDryRun && motors_.activeMotor() == MotorType::SUMP) {
    Serial.println(F("FAULT: sump motor dry run"));
    motors_.stop(MotorType::SUMP);
  }
}

void SystemController::printStatus() {
  Serial.print(F("mode="));
  Serial.print(toString(command_.mode));
  Serial.print(F(" overhead="));
  Serial.print(toString(sensorSnapshot_.overheadLevel));
  Serial.print(F(" sump="));
  Serial.print(toString(sensorSnapshot_.sumpLevel));
  Serial.print(F(" active_motor="));
  Serial.print(toString(motors_.activeMotor()));
  Serial.print(F(" borewell_status="));
  Serial.print(toString(motors_.stateOf(MotorType::BOREWELL).status));
  Serial.print(F(" sump_status="));
  Serial.print(toString(motors_.stateOf(MotorType::SUMP).status));
  Serial.print(F(" force_start="));
  Serial.println(command_.forceStart ? F("on") : F("off"));
}

bool SystemController::shouldFillToHigh() const {
  if (overheadAtOrAboveHigh()) {
    return false;
  }

  if (command_.mode == ControlMode::OVERRIDE_FILL || command_.forceStart) {
    return true;
  }

  return sensorSnapshot_.overheadLevel == OverheadLevel::LOW ||
         sensorSnapshot_.overheadLevel == OverheadLevel::CRITICAL ||
         sensorSnapshot_.overheadLevel == OverheadLevel::BELOW_CRITICAL;
}

bool SystemController::overheadAtOrAboveHigh() const {
  return sensorSnapshot_.overheadLevel == OverheadLevel::HIGH;
}

void SystemController::selectAndStartPreferredMotor(bool ignoreTankLevelLimit) {
  if (motors_.start(MotorType::BOREWELL, sensorSnapshot_, ignoreTankLevelLimit)) {
    return;
  }

  if (motors_.start(MotorType::SUMP, sensorSnapshot_, ignoreTankLevelLimit)) {
    return;
  }

  motors_.stopAll();
}
