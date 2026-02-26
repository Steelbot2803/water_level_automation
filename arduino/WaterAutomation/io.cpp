#include "io.h"

namespace {

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
  Serial.println(F("  override              -> fill till high regardless of OH level"));
  Serial.println(F("  motor borewell        -> manual: run borewell"));
  Serial.println(F("  motor sump            -> manual: run sump transfer"));
  Serial.println(F("  motor stop            -> stop motors"));
  Serial.println(F("  status                -> print current status"));
  Serial.println(F("  help                  -> show commands"));
}

}  // namespace

void printBanner() {
  Serial.println(F("\nWater Level Automation booted."));
  printHelp();
}

void readCommandFromSerial(SystemState& state) {
  const String line = readLine();
  if (line.isEmpty()) return;

  if (line == "help") {
    printHelp();
    return;
  }

  if (line == "auto") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = false;
    state.command.forcedMotor = MotorType::NONE;
    Serial.println(F("Mode set: AUTO"));
    return;
  }

  if (line == "manual") {
    state.command.manualMode = true;
    state.command.overrideFillToHigh = false;
    state.command.forcedMotor = MotorType::NONE;
    Serial.println(F("Mode set: MANUAL"));
    return;
  }

  if (line == "override") {
    state.command.manualMode = false;
    state.command.overrideFillToHigh = true;
    Serial.println(F("Override accepted: filling until overhead HIGH"));
    return;
  }

  if (line == "motor borewell") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::BOREWELL;
    Serial.println(F("Manual motor: BOREWELL"));
    return;
  }

  if (line == "motor sump") {
    state.command.manualMode = true;
    state.command.forcedMotor = MotorType::SUMP_TRANSFER;
    Serial.println(F("Manual motor: SUMP TRANSFER"));
    return;
  }

  if (line == "motor stop") {
    state.command.forcedMotor = MotorType::NONE;
    state.command.overrideFillToHigh = false;
    Serial.println(F("Motor command: STOP"));
    return;
  }

  if (line == "status") {
    state.lastStatusPrintMs = 0; // force status print in next status cycle
    return;
  }

  Serial.print(F("Unknown command: "));
  Serial.println(line);
}
