#pragma once

#include "config.h"

enum class MotorType : uint8_t {
  NONE,
  BOREWELL,
  SUMP
};

enum class MotorStatus : uint8_t {
  STOPPED,
  STARTING,
  RUNNING,
  DRY_RUN_LOCK,
  SUMP_CRITICAL  // sump motor specifically stopped because sump is critical
};

enum class OverheadLevel : uint8_t {
  CRITICAL,
  LOW,
  MEDIUM,
  HIGH
};

enum class SumpLevel : uint8_t {
  CRITICAL,
  LOW,
  HIGH
};

struct CommandState {
  bool manualMode = false;
  bool overrideFillToHigh = false;
  bool emergencyStop = false;               // stops everything, any mode
  bool autoPreferSump = false;              // force switch: in auto, try sump before borewell
  MotorType forcedMotor = MotorType::NONE;  // honored only in manual mode
};

struct MotorRuntimeState {
  MotorStatus status = MotorStatus::STOPPED;
  unsigned long startedAtMs = 0;
  unsigned long lockUntilMs = 0;
  bool dryRunLatched = false;
};

struct SystemState {
  OverheadLevel overheadLevel = OverheadLevel::LOW;
  SumpLevel sumpLevel = SumpLevel::LOW;

  CommandState command;

  MotorType activeMotor = MotorType::NONE;
  MotorRuntimeState borewell;
  MotorRuntimeState sump;

  bool sumpCriticalWarningLatched = false;
  unsigned long lastStatusPrintMs = 0;
};

void initState(SystemState& state);
void updateLevelsFromPins(SystemState& state);
void runAutomationLogic(SystemState& state);
void writeMotorOutputs(const SystemState& state);

void startMotor(SystemState& state, MotorType motor);
void stopMotor(MotorType motor);

void clearDryRunLatch(SystemState& state, MotorType motor);

const __FlashStringHelper* toText(MotorType m);
const __FlashStringHelper* toText(MotorStatus s);
const __FlashStringHelper* toText(OverheadLevel l);
const __FlashStringHelper* toText(SumpLevel l);