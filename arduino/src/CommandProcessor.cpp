#include "CommandProcessor.h"

void CommandProcessor::begin(unsigned long baud) {
  Serial.begin(baud);
  while (!Serial) {
    ;
  }
  printHelp();
}

void CommandProcessor::poll(UserCommand& command) {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (!line_.isEmpty()) {
        applyLine(line_, command);
        line_ = "";
      }
    } else {
      line_ += c;
    }
  }
}

void CommandProcessor::printHelp() const {
  Serial.println(F("Commands:"));
  Serial.println(F("  mode auto | mode override | mode manual"));
  Serial.println(F("  motor borewell | motor sump | motor none (manual mode only)"));
  Serial.println(F("  force on | force off"));
  Serial.println(F("  help"));
}

void CommandProcessor::applyLine(const String& line, UserCommand& command) {
  String raw = line;
  raw.trim();
  raw.toLowerCase();

  if (raw == "help") {
    printHelp();
    return;
  }
  if (raw == "mode auto") {
    command.mode = ControlMode::AUTO;
    command.forceStart = false;
    Serial.println(F("OK: mode=auto"));
    return;
  }
  if (raw == "mode override") {
    command.mode = ControlMode::OVERRIDE_FILL;
    Serial.println(F("OK: mode=override_fill"));
    return;
  }
  if (raw == "mode manual") {
    command.mode = ControlMode::MANUAL;
    Serial.println(F("OK: mode=manual"));
    return;
  }
  if (raw == "motor borewell") {
    command.manualMotor = MotorType::BOREWELL;
    Serial.println(F("OK: manual motor=borewell"));
    return;
  }
  if (raw == "motor sump") {
    command.manualMotor = MotorType::SUMP;
    Serial.println(F("OK: manual motor=sump"));
    return;
  }
  if (raw == "motor none") {
    command.manualMotor = MotorType::NONE;
    Serial.println(F("OK: manual motor=none"));
    return;
  }
  if (raw == "force on") {
    command.forceStart = true;
    Serial.println(F("OK: force start=on"));
    return;
  }
  if (raw == "force off") {
    command.forceStart = false;
    Serial.println(F("OK: force start=off"));
    return;
  }

  Serial.print(F("Unknown command: "));
  Serial.println(raw);
}
