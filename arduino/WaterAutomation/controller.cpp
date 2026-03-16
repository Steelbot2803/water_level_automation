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
  if (motor == MotorType::BOREWELL) return isPinActive(PIN_BOREWELL_FLOW_OK);
  if (motor == MotorType::SUMP) return isPinActive(PIN_SUMP_FLOW_OK);
  return false;
}

bool isLocked(const SystemState& state, MotorType motor, unsigned long nowMs) {
  const auto& rt = runtimeFor(state, motor);
  if (rt.dryRunLatched) return true;  // ← permanent latch wins
  return rt.lockUntilMs > nowMs;      // ← legacy timer path (now unreachable for dry-run)
}

void lockDryRun(SystemState& state, MotorType motor, unsigned long nowMs) {
  auto& rt = runtimeFor(state, motor);
  rt.status = MotorStatus::DRY_RUN_LOCK;
  rt.dryRunLatched = true;  // ← permanent until manually cleared
  rt.lockUntilMs = 0;       // ← timer disabled
  (void)nowMs;              // nowMs no longer needed here, suppress unused warning
}

void stopActiveMotor(SystemState& state) {
  if (state.activeMotor == MotorType::NONE) return;

  stopMotor(state.activeMotor);
  runtimeFor(state, state.activeMotor).status = MotorStatus::STOPPED;
  state.activeMotor = MotorType::NONE;
}

// Attempt to start a motor. requireSumpCheck gates the sump motor only.
// Returns true if the motor is running (or was already running).
bool tryStart(SystemState& state, MotorType motor, bool requireSumpCheck) {
  const unsigned long nowMs = millis();

  if (isLocked(state, motor, nowMs)) {
    if (state.activeMotor == motor) {
      stopActiveMotor(state);
    }
    return false;
  }

  if (requireSumpCheck && !sumpAllowsPumping(state)) {
    // Sump motor specifically: stop it if running, mark why.
    if (state.activeMotor == motor) {
      stopActiveMotor(state);
    }
    runtimeFor(state, motor).status = MotorStatus::SUMP_CRITICAL;
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

// In auto/override: try borewell first (unless force-switched), fall back to sump.
// Borewell is never blocked by sump level. Sump is blocked if sump is critical.
void selectAutoMotor(SystemState& state) {
  if (state.command.autoPreferSump) {
    // Force switch: try sump first, fall back to borewell.
    if (tryStart(state, MotorType::SUMP, true)) return;
    if (tryStart(state, MotorType::BOREWELL, false)) return;
  } else {
    // Normal priority: borewell > sump.
    if (tryStart(state, MotorType::BOREWELL, false)) return;
    if (tryStart(state, MotorType::SUMP, true)) return;
  }

  // Nothing could start. Borewell reports its actual reason.
  // (DRY_RUN_LOCK if locked, otherwise STOPPED — neither motor is at fault from sump.)
  if (isLocked(state, MotorType::BOREWELL, millis())) {
    state.borewell.status = MotorStatus::DRY_RUN_LOCK;
  }
  // Sump already has SUMP_CRITICAL set by tryStart if that was the reason.
}

void runManualControl(SystemState& state) {
  // Emergency stop takes absolute priority.
  if (state.command.emergencyStop) {
    stopActiveMotor(state);
    return;
  }

  if (state.command.forcedMotor == MotorType::NONE) {
    stopActiveMotor(state);
    return;
  }

  // Manual mode: sump critical does NOT block. The user explicitly chose this motor.
  // Dry-run protection still applies.
  if (!tryStart(state, state.command.forcedMotor, false)) {
    // Only reason tryStart can fail now is dry-run lock.
    // Status already set to DRY_RUN_LOCK by lockDryRun.
    return;
  }

  evaluateDryRun(state);
}

void runAutoControl(SystemState& state) {
  // Emergency stop takes absolute priority.
  if (state.command.emergencyStop) {
    stopActiveMotor(state);
    return;
  }

  // Sump critical: only stop the sump motor if it happens to be running.
  // Borewell is completely unaffected by sump level.
  if (state.sumpLevel == SumpLevel::CRITICAL) {
    state.sumpCriticalWarningLatched = true;
    if (state.activeMotor == MotorType::SUMP) {
      stopActiveMotor(state);
      state.sump.status = MotorStatus::SUMP_CRITICAL;
    }
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
    // Fallback: primary may have just dry-ran. Try again once.
    selectAutoMotor(state);
  }
}

}  // namespace

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
  digitalWrite(PIN_SUMP_RELAY, state.activeMotor == MotorType::SUMP ? HIGH : LOW);
}

void startMotor(SystemState&, MotorType) {
  // Output applied in writeMotorOutputs()
}

void stopMotor(MotorType motor) {
  if (motor == MotorType::BOREWELL) {
    digitalWrite(PIN_BOREWELL_RELAY, LOW);
  } else if (motor == MotorType::SUMP) {
    digitalWrite(PIN_SUMP_RELAY, LOW);
  }
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