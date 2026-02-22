#pragma once

#include <Arduino.h>

enum class OverheadLevel : uint8_t {
  BELOW_CRITICAL = 0,
  CRITICAL,
  LOW,
  MEDIUM,
  HIGH
};

enum class SumpLevel : uint8_t {
  BELOW_CRITICAL = 0,
  CRITICAL,
  LOW,
  HIGH
};

enum class MotorType : uint8_t {
  NONE = 0,
  BOREWELL,
  SUMP
};

enum class MotorStatus : uint8_t {
  STOPPED = 0,
  RUNNING,
  DRY_RUN,
  BLOCKED
};

enum class ControlMode : uint8_t {
  AUTO = 0,
  OVERRIDE_FILL,
  MANUAL
};

struct MotorState {
  MotorStatus status = MotorStatus::STOPPED;
  unsigned long startedAtMs = 0;
};

struct SensorSnapshot {
  OverheadLevel overheadLevel = OverheadLevel::HIGH;
  SumpLevel sumpLevel = SumpLevel::HIGH;
  bool borewellDryRun = false;
  bool sumpDryRun = false;
};

struct UserCommand {
  ControlMode mode = ControlMode::AUTO;
  MotorType manualMotor = MotorType::NONE;
  bool forceStart = false;
};

inline const __FlashStringHelper* toString(OverheadLevel level) {
  switch (level) {
    case OverheadLevel::BELOW_CRITICAL: return F("below_critical");
    case OverheadLevel::CRITICAL: return F("critical");
    case OverheadLevel::LOW: return F("low");
    case OverheadLevel::MEDIUM: return F("med");
    case OverheadLevel::HIGH: return F("high");
  }
  return F("unknown");
}

inline const __FlashStringHelper* toString(SumpLevel level) {
  switch (level) {
    case SumpLevel::BELOW_CRITICAL: return F("below_critical");
    case SumpLevel::CRITICAL: return F("critical");
    case SumpLevel::LOW: return F("low");
    case SumpLevel::HIGH: return F("high");
  }
  return F("unknown");
}

inline const __FlashStringHelper* toString(MotorType motor) {
  switch (motor) {
    case MotorType::NONE: return F("none");
    case MotorType::BOREWELL: return F("borewell");
    case MotorType::SUMP: return F("sump");
  }
  return F("unknown");
}

inline const __FlashStringHelper* toString(MotorStatus status) {
  switch (status) {
    case MotorStatus::STOPPED: return F("stopped");
    case MotorStatus::RUNNING: return F("running");
    case MotorStatus::DRY_RUN: return F("dry_run");
    case MotorStatus::BLOCKED: return F("blocked");
  }
  return F("unknown");
}

inline const __FlashStringHelper* toString(ControlMode mode) {
  switch (mode) {
    case ControlMode::AUTO: return F("auto");
    case ControlMode::OVERRIDE_FILL: return F("override_fill");
    case ControlMode::MANUAL: return F("manual");
  }
  return F("unknown");
}
