#include "Sensors.h"

void Sensors::begin() {
  pinMode(Pins::OH_CRITICAL, INPUT_PULLUP);
  pinMode(Pins::OH_LOW, INPUT_PULLUP);
  pinMode(Pins::OH_MED, INPUT_PULLUP);
  pinMode(Pins::OH_HIGH, INPUT_PULLUP);

  pinMode(Pins::SUMP_CRITICAL, INPUT_PULLUP);
  pinMode(Pins::SUMP_LOW, INPUT_PULLUP);
  pinMode(Pins::SUMP_HIGH, INPUT_PULLUP);

  pinMode(Pins::BOREWELL_DRY_RUN, INPUT_PULLUP);
  pinMode(Pins::SUMP_DRY_RUN, INPUT_PULLUP);
}

SensorSnapshot Sensors::read() const {
  SensorSnapshot s;

  const bool ohCritical = isWet(Pins::OH_CRITICAL);
  const bool ohLow = isWet(Pins::OH_LOW);
  const bool ohMed = isWet(Pins::OH_MED);
  const bool ohHigh = isWet(Pins::OH_HIGH);

  if (ohHigh) {
    s.overheadLevel = OverheadLevel::HIGH;
  } else if (ohMed) {
    s.overheadLevel = OverheadLevel::MEDIUM;
  } else if (ohLow) {
    s.overheadLevel = OverheadLevel::LOW;
  } else if (ohCritical) {
    s.overheadLevel = OverheadLevel::CRITICAL;
  } else {
    s.overheadLevel = OverheadLevel::BELOW_CRITICAL;
  }

  const bool sumpCritical = isWet(Pins::SUMP_CRITICAL);
  const bool sumpLow = isWet(Pins::SUMP_LOW);
  const bool sumpHigh = isWet(Pins::SUMP_HIGH);

  if (sumpHigh) {
    s.sumpLevel = SumpLevel::HIGH;
  } else if (sumpLow) {
    s.sumpLevel = SumpLevel::LOW;
  } else if (sumpCritical) {
    s.sumpLevel = SumpLevel::CRITICAL;
  } else {
    s.sumpLevel = SumpLevel::BELOW_CRITICAL;
  }

  s.borewellDryRun = digitalRead(Pins::BOREWELL_DRY_RUN) == LOW;
  s.sumpDryRun = digitalRead(Pins::SUMP_DRY_RUN) == LOW;

  return s;
}

bool Sensors::isWet(uint8_t pin) {
  return digitalRead(pin) == LOW;
}
