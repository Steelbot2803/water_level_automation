#include "controller.h"

namespace {

bool isPinActive(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

OverheadLevel readOverheadLevel() {
  if (isPinActive(PIN_OH_HIGH)) return OverheadLevel::HIGH;
  if (isPinActive(PIN_OH_MED)) return OverheadLevel::MEDIUM;
  if (isPinActive(PIN_OH_LOW)) return OverheadLevel::LOW;

  // Below LOW is treated as CRITICAL.
  return OverheadLevel::CRITICAL;
}

SumpLevel readSumpLevel() {
  if (isPinActive(PIN_SUMP_HIGH)) return SumpLevel::HIGH;
  if (isPinActive(PIN_SUMP_LOW)) return SumpLevel::LOW;

  // Below LOW is treated as CRITICAL.
  return SumpLevel::CRITICAL;
}

bool needsFill(const SystemState& state) {
  return state.overheadLevel == OverheadLevel::LOW ||
         state.overheadLevel == OverheadLevel::CRITICAL ||
         state.overheadLevel == OverheadLevel::EMPTY;
}

bool reachedStopLevel(const SystemState& state) {
  return state.overheadLevel == OverheadLevel::HIGH;
}

bool sumpAllowsPumping(const SystemState& state) {
  return state.sumpLevel != SumpLevel::CRITICAL && state.sumpLevel != SumpLevel::BELOW_CRITICAL;
}

MotorRuntimeState& runtimeFor(SystemState& state, MotorType motor) {
  return (motor == MotorType::BOREWELL) ? state.borewell : state.sumpTransfer;
}

const MotorRuntimeState& runtimeFor(const SystemState& state, MotorType motor) {
  return (motor == MotorType::BOREWELL) ? state.borewell : state.sumpTransfer;
}

bool flowOkay(MotorType motor) {
  if (motor == MotorType::BOREWELL) return isPinActive(PIN_BOREWELL_FLOW_OK);
  if (motor == MotorType::SUMP_TRANSFER) return isPinActive(PIN_SUMP_FLOW_OK);
  return false;
}

bool isLocked(const SystemState& state, MotorType motor, unsigned long nowMs) {
  return runtimeFor(state, motor).lockUntilMs > nowMs;
}

void lockDryRun(SystemState& state, MotorType motor, unsigned long nowMs) {
  auto& rt = runtimeFor(state, motor);
  rt.status = MotorStatus::DRY_RUN_LOCK;
  rt.lockUntilMs = nowMs + START_RETRY_LOCK_MS;
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
    if (state.activeMotor == motor) {
      stopActiveMotor(state);
    }
    return false;
  }

  if (requireSumpCheck && !sumpAllowsPumping(state)) {
    if (state.activeMotor == motor) {
      stopActiveMotor(state);
    }
    return false;
  }

  if (state.activeMotor == motor) {
    return true;
  }

  stopActiveMotor(state);

  startMotor(state, motor);
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

  if (rt.status == MotorStatus::STARTING && nowMs - rt.startedAtMs < DRY_RUN_GRACE_MS) {
    return;
  }

  if (flowOkay(state.activeMotor)) {
    rt.status = MotorStatus::RUNNING;
    return;
  }

  const MotorType failedMotor = state.activeMotor;
  stopActiveMotor(state);
  lockDryRun(state, failedMotor, nowMs);
}

void selectAutoMotor(SystemState& state) {
  // Priority: borewell > sump transfer.
  if (tryStart(state, MotorType::BOREWELL, false)) return;
  if (tryStart(state, MotorType::SUMP_TRANSFER, true)) return;

  // Nothing could start.
  state.borewell.status = isLocked(state, MotorType::BOREWELL, millis())
                              ? MotorStatus::DRY_RUN_LOCK
                              : MotorStatus::BLOCKED_BY_SAFETY;
  state.sumpTransfer.status = MotorStatus::BLOCKED_BY_SAFETY;
}

void runManualControl(SystemState& state) {
  if (state.command.forcedMotor == MotorType::NONE) {
    stopActiveMotor(state);
    return;
  }

  const bool requiresSump = state.command.forcedMotor == MotorType::SUMP_TRANSFER;
  if (!tryStart(state, state.command.forcedMotor, requiresSump)) {
    runtimeFor(state, state.command.forcedMotor).status = MotorStatus::BLOCKED_BY_SAFETY;
    return;
  }

  evaluateDryRun(state);
}

void runAutoControl(SystemState& state) {
  if (state.sumpLevel == SumpLevel::CRITICAL) {
    state.sumpCriticalWarningLatched = true;
    stopActiveMotor(state);
    state.sumpTransfer.status = MotorStatus::BLOCKED_BY_SAFETY;
    if (!state.command.overrideFillToHigh) return;
  }

  const bool shouldFill = state.command.overrideFillToHigh || needsFill(state);

  if (!shouldFill) {
    stopActiveMotor(state);
    return;
  }

  if (reachedStopLevel(state)) {
    stopActiveMotor(state);
    state.command.overrideFillToHigh = false;
    return;
  }

  if (state.activeMotor == MotorType::NONE) {
    selectAutoMotor(state);
  }

  evaluateDryRun(state);

  if (state.activeMotor == MotorType::NONE) {
    // fallback once more in case primary dry-ran just now
    selectAutoMotor(state);
  }
}

} // namespace

void initState(SystemState& state) {
  pinMode(PIN_OH_LOW, INPUT_PULLUP);
  pinMode(PIN_OH_MED, INPUT_PULLUP);
  pinMode(PIN_OH_HIGH, INPUT_PULLUP);

  pinMode(PIN_SUMP_LOW, INPUT_PULLUP);
  pinMode(PIN_SUMP_HIGH, INPUT_PULLUP);

  updateLevelsFromPins(state);
}

void updateLevelsFromPins(SystemState& state) {
  state.overheadLevel = readOverheadLevel();
  state.sumpLevel = readSumpLevel();
}

void runAutomationLogic(SystemState& state) {
  if (state.command.manualMode) {
    runManualControl(state);
  } else {
    runAutoControl(state);
  }
}

void writeMotorOutputs(const SystemState& state) {
  digitalWrite(PIN_BOREWELL_RELAY, state.activeMotor == MotorType::BOREWELL ? HIGH : LOW);
  digitalWrite(PIN_SUMP_RELAY, state.activeMotor == MotorType::SUMP_TRANSFER ? HIGH : LOW);
}

void startMotor(SystemState&, MotorType) {
  // Output applied in writeMotorOutputs()
}

void stopMotor(MotorType motor) {
  if (motor == MotorType::BOREWELL) {
    digitalWrite(PIN_BOREWELL_RELAY, LOW);
  } else if (motor == MotorType::SUMP_TRANSFER) {
    digitalWrite(PIN_SUMP_RELAY, LOW);
  }
}

const __FlashStringHelper* toText(MotorType m) {
  switch (m) {
    case MotorType::NONE: return F("none");
    case MotorType::BOREWELL: return F("borewell");
    case MotorType::SUMP_TRANSFER: return F("sump_transfer");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(MotorStatus s) {
  switch (s) {
    case MotorStatus::STOPPED: return F("stopped");
    case MotorStatus::STARTING: return F("starting");
    case MotorStatus::RUNNING: return F("running");
    case MotorStatus::DRY_RUN_LOCK: return F("dry_run_lock");
    case MotorStatus::BLOCKED_BY_SAFETY: return F("blocked_by_safety");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(OverheadLevel l) {
  switch (l) {
    case OverheadLevel::EMPTY: return F("empty");
    case OverheadLevel::CRITICAL: return F("critical");
    case OverheadLevel::LOW: return F("low");
    case OverheadLevel::MEDIUM: return F("medium");
    case OverheadLevel::HIGH: return F("high");
  }
  return F("unknown");
}

const __FlashStringHelper* toText(SumpLevel l) {
  switch (l) {
    case SumpLevel::BELOW_CRITICAL: return F("below_critical");
    case SumpLevel::CRITICAL: return F("critical");
    case SumpLevel::LOW: return F("low");
    case SumpLevel::HIGH: return F("high");
  }
  return F("unknown");
}
