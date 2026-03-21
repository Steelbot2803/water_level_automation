#include "io.h"
#include "persistence.h"

namespace {

SystemState* gState = nullptr;

String readLine() {
  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (line.length() == 0) continue;
      String out = line;
      line = "";
      out.trim();
      out.toLowerCase();
      return out;
    }
    line += c;
  }
  return "";
}

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  auto                  -> automatic mode"));
  Serial.println(F("  manual                -> manual mode"));
  Serial.println(F("  override              -> fill to HIGH regardless of overhead level"));
  Serial.println(F("  motor borewell        -> manual: run borewell motor"));
  Serial.println(F("  motor sump            -> manual: run sump transfer motor"));
  Serial.println(F("  motor stop            -> stop active motor (any mode)"));
  Serial.println(F("  borewell        -> auto: prefer borewell (default)"));
  Serial.println(F("  sump            -> auto: prefer sump over borewell"));
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
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Mode set: AUTO"));
    return true;
  }

  if (line == "manual") {
    state.command.manualMode = true;
    state.command.overrideFillToHigh = false;
    state.command.forcedMotor = MotorType::NONE;
    state.command.emergencyStop = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Mode set: MANUAL"));
    return true;
  }

  if (line == "override") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = true;
    state.command.emergencyStop = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Override accepted: filling until overhead HIGH"));
    return true;
  }

  if (line == "motor borewell") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::BOREWELL;
    state.command.emergencyStop = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Manual motor: BOREWELL"));
    return true;
  }

  if (line == "motor sump") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::SUMP;
    state.command.emergencyStop = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Manual motor: SUMP TRANSFER"));
    return true;
  }

  // Motor stop: works in any mode — clears forced motor and override,
  // but does NOT change mode so the user stays where they are.
  if (line == "motor stop") {
    state.command.forcedMotor = MotorType::NONE;
    state.command.overrideFillToHigh = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Motor stop commanded"));
    return true;
  }

  // Force switch: sets preferred motor for auto selection without leaving auto mode.
  if (line == "borewell") {
    state.command.autoPreferSump = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Auto preference: BOREWELL (default)"));
    return true;
  }

  if (line == "sump") {
    state.command.autoPreferSump = true;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("Auto preference: SUMP TRANSFER"));
    return true;
  }

  // Emergency stop: stops everything immediately, any mode.
  if (line == "estop") {
    state.command.emergencyStop = true;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("EMERGENCY STOP"));
    return true;
  }

  if (line == "resume") {
    state.command.emergencyStop = false;
    persistCommandStateIfChanged(state.command);
    Serial.println(F("RESUME OPERATIONS"));
    return true;
  }

  if (line == "status") {
    state.lastStatusPrintMs = 0;  // force status print in next cycle
    return true;
  }

  if (line == "reset") {
    persistCommandStateIfChanged(state.command);
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
    persistCommandStateIfChanged(state.command);
    Serial.println(F("State reset to defaults"));
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
