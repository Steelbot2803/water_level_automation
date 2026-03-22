#include "controller.h"
#include "persistence.h"
#include "probes.h"

namespace {

OverheadLevel readOverheadLevel() {
  if (acProbeActive(PIN_OH_HIGH)) return OverheadLevel::HIGH;
  if (acProbeActive(PIN_OH_MED)) return OverheadLevel::MEDIUM;
  if (acProbeActive(PIN_OH_LOW)) return OverheadLevel::LOW;
  return OverheadLevel::CRITICAL;
}

SumpLevel readSumpLevel() {
  if (acProbeActive(PIN_SUMP_HIGH)) return SumpLevel::HIGH;
  if (acProbeActive(PIN_SUMP_LOW)) return SumpLevel::LOW;
  return SumpLevel::CRITICAL;
}

bool needsFill(const SystemState& state) {
  return state.overheadLevel == OverheadLevel::LOW
         || state.overheadLevel == OverheadLevel::CRITICAL;
}

bool reachedStopLevel(const SystemState& state) {
  return state.overheadLevel == OverheadLevel::HIGH;
}

bool sumpAllowsPumping(const SystemState& state) {
  return state.sumpLevel != SumpLevel::CRITICAL;
}

MotorRuntimeState& runtimeFor(SystemState& state, MotorType motor) {
  return (motor == MotorType::BOREWELL) ? state.borewell : state.sump;
}

const MotorRuntimeState& runtimeFor(const SystemState& state, MotorType motor) {
  return (motor == MotorType::BOREWELL) ? state.borewell : state.sump;
}

bool flowOkay(MotorType motor) {
  const uint8_t pin = (motor == MotorType::BOREWELL)
                        ? PIN_BOREWELL_CURRENT
                        : PIN_SUMP_CURRENT;

  int peakDeviation = 0;
  const unsigned long endMs = millis() + 30;

  while (millis() < endMs) {
    const int sample = analogRead(pin);
    const int deviation = abs(sample - 512);
    if (deviation > peakDeviation) {
      peakDeviation = deviation;
    }
  }

  return peakDeviation > MOTOR_CURRENT_THRESHOLD;
}

bool isLocked(const SystemState& state, MotorType motor, unsigned long nowMs) {
  const auto& rt = runtimeFor(state, motor);
  if (rt.dryRunLatched) return true;
  return rt.lockUntilMs > nowMs;
}

void lockDryRun(SystemState& state, MotorType motor, unsigned long nowMs) {
  auto& rt = runtimeFor(state, motor);
  rt.status = MotorStatus::DRY_RUN_LOCK;
  rt.dryRunLatched = true;
  rt.lockUntilMs = 0;
  (void)nowMs;
}

void stopActiveMotor(SystemState& state) {
  if (state.activeMotor == MotorType::NONE) return;
  stopMotor(state.activeMotor);
  runtimeFor(state, state.activeMotor).status = MotorStatus::STOPPED;
  state.activeMotor = MotorType::NONE;
}

bool tryStart(SystemState& state, MotorType motor, bool requireSumpCheck) {
  const unsigned long nowMs = millis();

  if (isLocked(state, motor, nowMs)) {
    if (state.activeMotor == motor) stopActiveMotor(state);
    return false;
  }

  if (requireSumpCheck && !sumpAllowsPumping(state)) {
    if (state.activeMotor == motor) stopActiveMotor(state);
    runtimeFor(state, motor).status = MotorStatus::SUMP_CRITICAL;
    return false;
  }

  if (state.activeMotor == motor) return true;

  stopActiveMotor(state);
  auto& rt = runtimeFor(state, motor);
  rt.status = MotorStatus::STARTING;
  rt.startedAtMs = nowMs;
  state.activeMotor = motor;
  return true;
}

void evaluateDryRun(SystemState& state) {
  if (state.activeMotor == MotorType::NONE) return;

  const unsigned long nowMs = millis();
  auto& rt = runtimeFor(state, state.activeMotor);

  if (rt.status == MotorStatus::STARTING && nowMs - rt.startedAtMs < DRY_RUN_GRACE_MS) return;

  if (flowOkay(state.activeMotor)) {
    rt.status = MotorStatus::RUNNING;
    return;
  }

  const MotorType failedMotor = state.activeMotor;
  stopActiveMotor(state);
  lockDryRun(state, failedMotor, nowMs);
}

void selectAutoMotor(SystemState& state) {
  if (state.command.autoPreferSump) {
    if (tryStart(state, MotorType::SUMP, true)) return;
    if (tryStart(state, MotorType::BOREWELL, false)) return;
  } else {
    if (tryStart(state, MotorType::BOREWELL, false)) return;
    if (tryStart(state, MotorType::SUMP, true)) return;
  }
  if (isLocked(state, MotorType::BOREWELL, millis())) {
    state.borewell.status = MotorStatus::DRY_RUN_LOCK;
  }
  if (isLocked(state, MotorType::SUMP, millis()) && state.sump.status != MotorStatus::SUMP_CRITICAL) {
    state.sump.status = MotorStatus::DRY_RUN_LOCK;
  }
}

void runManualControl(SystemState& state) {
  if (state.command.emergencyStop) {
    stopActiveMotor(state);
    return;
  }
  if (state.command.forcedMotor == MotorType::NONE) {
    stopActiveMotor(state);
    return;
  }
  if (!tryStart(state, state.command.forcedMotor, false)) return;
  evaluateDryRun(state);
}

void runAutoControl(SystemState& state) {
  if (state.command.emergencyStop) {
    stopActiveMotor(state);
    state.fillCycleActive = false;
    return;
  }

  if (state.sumpLevel == SumpLevel::CRITICAL) {
    state.sumpCriticalWarningLatched = true;
    if (state.activeMotor == MotorType::SUMP) {
      stopActiveMotor(state);
      state.sump.status = MotorStatus::SUMP_CRITICAL;
    }
  }

  if (needsFill(state)) {
    state.fillCycleActive = true;
  }

  const bool shouldFill = state.command.overrideFillToHigh || state.fillCycleActive;

  if (!shouldFill) {
    stopActiveMotor(state);
    return;
  }

  if (reachedStopLevel(state)) {
    stopActiveMotor(state);
    state.fillCycleActive = false;
    state.command.overrideFillToHigh = false;
    return;
  }

  if (state.activeMotor != MotorType::NONE) {
    const bool preferSump = state.command.autoPreferSump;
    const bool runningWrongMotor =
      (preferSump && state.activeMotor == MotorType::BOREWELL) || (!preferSump && state.activeMotor == MotorType::SUMP);

    if (runningWrongMotor) {
      const MotorType preferred = preferSump ? MotorType::SUMP : MotorType::BOREWELL;
      const unsigned long nowMs = millis();
      const bool preferredAvailable =
        !isLocked(state, preferred, nowMs) && !(preferSump && !sumpAllowsPumping(state));

      if (preferredAvailable) {
        stopActiveMotor(state);
      }
    }
  }

  if (state.activeMotor == MotorType::NONE) selectAutoMotor(state);
  evaluateDryRun(state);
  if (state.activeMotor == MotorType::NONE) selectAutoMotor(state);
}

}

void initState(SystemState& state) {
  initProbes();
  loadPersistedState(state);
  updateLevelsFromPins(state);
}

void updateLevelsFromPins(SystemState& state) {
  state.overheadLevel = readOverheadLevel();
  state.sumpLevel = readSumpLevel();
}

void runAutomationLogic(SystemState& state) {
  if (state.command.manualMode) runManualControl(state);
  else runAutoControl(state);
  persistStateIfChanged(state);
}

void writeMotorOutputs(const SystemState& state) {
  digitalWrite(PIN_BOREWELL_RELAY, state.activeMotor == MotorType::BOREWELL ? HIGH : LOW);
  digitalWrite(PIN_SUMP_RELAY, state.activeMotor == MotorType::SUMP ? HIGH : LOW);
}

void stopMotor(MotorType motor) {
  if (motor == MotorType::BOREWELL) digitalWrite(PIN_BOREWELL_RELAY, LOW);
  else if (motor == MotorType::SUMP) digitalWrite(PIN_SUMP_RELAY, LOW);
}

void clearDryRunLatch(SystemState& state, MotorType motor) {
  auto& rt = runtimeFor(state, motor);
  rt.dryRunLatched = false;
  rt.lockUntilMs = 0;
  rt.status = MotorStatus::STOPPED;
}

const __FlashStringHelper* toText(MotorType m) {
  switch (m) {
    case MotorType::NONE: return F("none");
    case MotorType::BOREWELL: return F("borewell");
    case MotorType::SUMP: return F("sump");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(MotorStatus s) {
  switch (s) {
    case MotorStatus::STOPPED: return F("stopped");
    case MotorStatus::STARTING: return F("starting");
    case MotorStatus::RUNNING: return F("running");
    case MotorStatus::DRY_RUN_LOCK: return F("dry_run_lock");
    case MotorStatus::SUMP_CRITICAL: return F("sump_critical");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(OverheadLevel l) {
  switch (l) {
    case OverheadLevel::CRITICAL: return F("critical");
    case OverheadLevel::LOW: return F("low");
    case OverheadLevel::MEDIUM: return F("medium");
    case OverheadLevel::HIGH: return F("high");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(SumpLevel l) {
  switch (l) {
    case SumpLevel::CRITICAL: return F("critical");
    case SumpLevel::LOW: return F("low");
    case SumpLevel::HIGH: return F("high");
  }
  return F("unknown");
}

const char* toStr(MotorType m) {
  switch (m) {
    case MotorType::NONE: return "none";
    case MotorType::BOREWELL: return "borewell";
    case MotorType::SUMP: return "sump";
  }
  return "unknown";
}

const char* toStr(MotorStatus s) {
  switch (s) {
    case MotorStatus::STOPPED: return "stopped";
    case MotorStatus::STARTING: return "starting";
    case MotorStatus::RUNNING: return "running";
    case MotorStatus::DRY_RUN_LOCK: return "dry_run_lock";
    case MotorStatus::SUMP_CRITICAL: return "sump_critical";
  }
  return "unknown";
}

const char* toStr(OverheadLevel l) {
  switch (l) {
    case OverheadLevel::CRITICAL: return "critical";
    case OverheadLevel::LOW: return "low";
    case OverheadLevel::MEDIUM: return "medium";
    case OverheadLevel::HIGH: return "high";
  }
  return "unknown";
}

const char* toStr(SumpLevel l) {
  switch (l) {
    case SumpLevel::CRITICAL: return "critical";
    case SumpLevel::LOW: return "low";
    case SumpLevel::HIGH: return "high";
  }
  return "unknown";
}
