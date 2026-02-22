#pragma once

#include "config.h"

enum class MotorType : uint8_t {
  NONE,
  BOREWELL,
  SUMP_TRANSFER
};

enum class MotorStatus : uint8_t {
  STOPPED,
  STARTING,
  RUNNING,
  DRY_RUN_LOCK,
  BLOCKED_BY_SAFETY
};

enum class OverheadLevel : uint8_t {
  EMPTY,
  CRITICAL,
  LOW,
  MEDIUM,
  HIGH
};

enum class SumpLevel : uint8_t {
  BELOW_CRITICAL,
  CRITICAL,
  LOW,
  HIGH
};

struct CommandState {
  bool manualMode = false;
  bool overrideFillToHigh = false;
  MotorType forcedMotor = MotorType::NONE; // honored only in manual mode
};

struct MotorRuntimeState {
  MotorStatus status = MotorStatus::STOPPED;
  unsigned long startedAtMs = 0;
  unsigned long lockUntilMs = 0;
};

struct SystemState {
  OverheadLevel overheadLevel = OverheadLevel::EMPTY;
  SumpLevel sumpLevel = SumpLevel::LOW;

  CommandState command;

  MotorType activeMotor = MotorType::NONE;
  MotorRuntimeState borewell;
  MotorRuntimeState sumpTransfer;

  bool sumpCriticalWarningLatched = false;
  unsigned long lastStatusPrintMs = 0;
};

void initState(SystemState& state);
void updateLevelsFromPins(SystemState& state);
void runAutomationLogic(SystemState& state);
void writeMotorOutputs(const SystemState& state);

void startMotor(SystemState& state, MotorType motor);
void stopMotor(MotorType motor);

const __FlashStringHelper* toText(MotorType m);
const __FlashStringHelper* toText(MotorStatus s);
const __FlashStringHelper* toText(OverheadLevel l);
const __FlashStringHelper* toText(SumpLevel l);
