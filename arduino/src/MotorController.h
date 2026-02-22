#pragma once

#include "Config.h"
#include "Types.h"

class MotorController {
 public:
  void begin();
  bool start(MotorType motor, const SensorSnapshot& sensors, bool ignoreTankLevelLimit);
  void stop(MotorType motor);
  void stopAll();
  MotorType activeMotor() const { return activeMotor_; }
  MotorState stateOf(MotorType motor) const;

 private:
  MotorType activeMotor_ = MotorType::NONE;
  MotorState borewell_;
  MotorState sump_;

  bool canStart(MotorType motor, const SensorSnapshot& sensors, bool ignoreTankLevelLimit) const;
  MotorState& stateRef(MotorType motor);
  uint8_t relayPin(MotorType motor) const;
  bool dryRunTriggered(MotorType motor, const SensorSnapshot& sensors) const;
};
