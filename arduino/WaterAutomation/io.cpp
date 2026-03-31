#include "io.h"
#include "persistence.h"

namespace {

SystemState* gState = nullptr;
constexpr size_t MAX_COMMAND_LEN = 96;

// Fixed char buffer replaces Arduino String — no heap allocation,
// no fragmentation. Index tracks how many bytes have been written.
String readLine() {
  static char line[MAX_COMMAND_LEN + 1];
  static size_t idx = 0;

  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\n' || c == '\r') {
      if (idx == 0) continue;
      line[idx] = '\0';
      idx = 0;

      // Trim trailing whitespace in-place.
      size_t len = strlen(line);
      while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) line[--len] = '\0';

      // Lower-case in-place.
      for (size_t i = 0; i < len; ++i)
        if (line[i] >= 'A' && line[i] <= 'Z') line[i] += 32;

      return String(line);  // one heap alloc at dispatch time, not during accumulation
    }

    if (idx < MAX_COMMAND_LEN) {
      line[idx++] = c;
    } else {
      idx = 0;
      Serial.println(F("Command too long. Input discarded."));
    }
  }
  return String();
}

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  auto                  -> automatic mode"));
  Serial.println(F("  manual                -> manual mode"));
  Serial.println(F("  override              -> fill to HIGH regardless of overhead level"));
  Serial.println(F("  motor borewell        -> manual: run borewell motor"));
  Serial.println(F("  motor sump            -> manual: run sump motor"));
  Serial.println(F("  motor stop            -> stop active motor (any mode)"));
  Serial.println(F("  borewell              -> auto: prefer borewell (default)"));
  Serial.println(F("  sump                  -> auto: prefer sump over borewell"));
  Serial.println(F("  estop                 -> emergency stop all motors"));
  Serial.println(F("  status                -> print current status"));
  Serial.println(F("  help                  -> show commands"));
}

}  // namespace

void setSystemStateRef(SystemState* state) {
  gState = state;
}
SystemState* getSystemStateRef() {
  return gState;
}

void printBanner() {
  Serial.println(F("\nWater Level Automation booted."));
  printHelp();
}

bool applyCommand(SystemState& state, const String& line) {
  if (line == "help") {
    printHelp();
    return true;
  }

  if (line == "auto") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = false;
    state.command.forcedMotor = MotorType::NONE;
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("Mode set: AUTO"));
    return true;
  }

  if (line == "manual") {
    state.command.manualMode = true;
    state.command.overrideFillToHigh = false;
    state.command.forcedMotor = MotorType::NONE;
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("Mode set: MANUAL"));
    return true;
  }

  if (line == "override") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = true;
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("Override accepted: filling until overhead HIGH"));
    return true;
  }

  if (line == "motor borewell") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::BOREWELL;
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("Manual motor: BOREWELL"));
    return true;
  }

  if (line == "motor sump") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::SUMP;
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("Manual motor: SUMP"));
    return true;
  }

  if (line == "motor stop") {
    state.command.forcedMotor = MotorType::NONE;
    state.command.overrideFillToHigh = false;
    persistStateIfChanged(state);
    Serial.println(F("Motor stop commanded"));
    return true;
  }

  if (line == "borewell") {
    state.command.autoPreferSump = false;
    persistStateIfChanged(state);
    Serial.println(F("Auto preference: BOREWELL (default)"));
    return true;
  }

  if (line == "sump") {
    state.command.autoPreferSump = true;
    persistStateIfChanged(state);
    Serial.println(F("Auto preference: SUMP"));
    return true;
  }

  if (line == "estop") {
    state.command.emergencyStop = true;
    persistStateIfChanged(state);
    Serial.println(F("EMERGENCY STOP"));
    return true;
  }

  if (line == "resume") {
    state.command.emergencyStop = false;
    persistStateIfChanged(state);
    Serial.println(F("RESUME OPERATIONS"));
    return true;
  }

  if (line == "status") {
    state.lastStatusPrintMs = 0;
    return true;
  }

  if (line == "reset") {
    persistStateIfChanged(state);
    Serial.println(F("Resetting by command..."));
    delay(100);
    NVIC_SystemReset();
    return true;
  }

  if (line == "unlock borewell") {
    clearDryRunLatch(state, MotorType::BOREWELL);
    Serial.println(F("Borewell dry-run latch cleared"));
    return true;
  }

  if (line == "unlock sump") {
    clearDryRunLatch(state, MotorType::SUMP);
    Serial.println(F("Sump dry-run latch cleared"));
    return true;
  }

  if (line == "read current") {
    long sumB = 0, sumS = 0;
    for (int i = 0; i < 16; i++) {
      sumB += analogRead(PIN_BOREWELL_CURRENT);
      sumS += analogRead(PIN_SUMP_CURRENT);
      delay(10);
    }
    Serial.print(F("Borewell current ADC avg: "));
    Serial.println(sumB / 16);
    Serial.print(F("Sump current ADC avg: "));
    Serial.println(sumS / 16);
    return true;
  }

  if (line == "reset state") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = false;
    state.command.emergencyStop = false;
    state.command.autoPreferSump = false;
    state.command.forcedMotor = MotorType::NONE;
    state.fillCycleActive = false;
    persistStateIfChanged(state);
    Serial.println(F("State reset to defaults"));
    return true;
  }

  if (line == "sync") {
    state.syncRequested = true;
    Serial.println(F("sync requested"));
    return true;
  }

  return false;
}

void readCommandFromSerial(SystemState& state) {
  const String line = readLine();
  if (line.isEmpty()) return;
  if (!applyCommand(state, line)) {
    Serial.print(F("Unknown command: "));
    Serial.println(line);
  }
}