#pragma once

#include "CommandProcessor.h"
#include "MotorController.h"
#include "Sensors.h"

class SystemController {
 public:
  void begin();
  void loop();

 private:
  Sensors sensors_;
  MotorController motors_;
  CommandProcessor commands_;
  UserCommand command_;
  SensorSnapshot sensorSnapshot_;
  unsigned long lastStatusAtMs_ = 0;

  void runLogic();
  void enforceDryRunSafety();
  void printStatus();
  bool shouldFillToHigh() const;
  bool overheadAtOrAboveHigh() const;
  void selectAndStartPreferredMotor(bool ignoreTankLevelLimit);
};
