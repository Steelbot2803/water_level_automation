#include "persistence.h"

#include <Arduino.h>
#include <EEPROM.h>

namespace {

constexpr int ADDR_MAGIC_0 = 0;
constexpr int ADDR_MAGIC_1 = 1;
constexpr int ADDR_VERSION = 2;
constexpr int ADDR_MANUAL_MODE = 3;
constexpr int ADDR_OVERRIDE = 4;
constexpr int ADDR_ESTOP = 5;
constexpr int ADDR_PREFER_SUMP = 6;
constexpr int ADDR_FORCED_MOTOR = 7;
constexpr int ADDR_CHECKSUM = 8;

constexpr uint8_t MAGIC_0 = 0x57; // W
constexpr uint8_t MAGIC_1 = 0x4C; // L
constexpr uint8_t VERSION = 0x01;

bool persistenceReady = false;
CommandState lastSavedCommand = {};
bool haveLastSavedCommand = false;

uint8_t toByte(bool value) {
  return value ? 1 : 0;
}

uint8_t checksumFor(
  uint8_t manualMode,
  uint8_t overrideFillToHigh,
  uint8_t emergencyStop,
  uint8_t autoPreferSump,
  uint8_t forcedMotor
) {
  return static_cast<uint8_t>(
    MAGIC_0 ^ MAGIC_1 ^ VERSION ^ manualMode ^ overrideFillToHigh ^ emergencyStop ^ autoPreferSump ^ forcedMotor
  );
}

bool sameCommandState(const CommandState& a, const CommandState& b) {
  return a.manualMode == b.manualMode
      && a.overrideFillToHigh == b.overrideFillToHigh
      && a.emergencyStop == b.emergencyStop
      && a.autoPreferSump == b.autoPreferSump
      && a.forcedMotor == b.forcedMotor;
}

void applyDefaults(CommandState& command) {
  command.manualMode = false;
  command.overrideFillToHigh = false;
  command.emergencyStop = false;
  command.autoPreferSump = false;
  command.forcedMotor = MotorType::NONE;
}

}

void initPersistence() {
  persistenceReady = true;
}

void loadPersistedCommandState(CommandState& command) {
  if (!persistenceReady) initPersistence();

  const uint8_t magic0 = EEPROM.read(ADDR_MAGIC_0);
  const uint8_t magic1 = EEPROM.read(ADDR_MAGIC_1);
  const uint8_t version = EEPROM.read(ADDR_VERSION);
  const uint8_t manualMode = EEPROM.read(ADDR_MANUAL_MODE);
  const uint8_t overrideFillToHigh = EEPROM.read(ADDR_OVERRIDE);
  const uint8_t emergencyStop = EEPROM.read(ADDR_ESTOP);
  const uint8_t autoPreferSump = EEPROM.read(ADDR_PREFER_SUMP);
  const uint8_t forcedMotor = EEPROM.read(ADDR_FORCED_MOTOR);
  const uint8_t checksum = EEPROM.read(ADDR_CHECKSUM);

  const bool valid =
    magic0 == MAGIC_0 &&
    magic1 == MAGIC_1 &&
    version == VERSION &&
    forcedMotor <= static_cast<uint8_t>(MotorType::SUMP) &&
    checksum == checksumFor(manualMode, overrideFillToHigh, emergencyStop, autoPreferSump, forcedMotor);

  if (!valid) {
    applyDefaults(command);
    lastSavedCommand = command;
    haveLastSavedCommand = true;
    Serial.println(F("persistence: no valid saved command state, using defaults"));
    return;
  }

  command.manualMode = manualMode != 0;
  command.overrideFillToHigh = overrideFillToHigh != 0;
  command.emergencyStop = emergencyStop != 0;
  command.autoPreferSump = autoPreferSump != 0;
  command.forcedMotor = static_cast<MotorType>(forcedMotor);

  lastSavedCommand = command;
  haveLastSavedCommand = true;
  Serial.println(F("persistence: restored saved command state"));
}

void persistCommandStateIfChanged(const CommandState& command) {
  if (!persistenceReady) initPersistence();
  if (haveLastSavedCommand && sameCommandState(command, lastSavedCommand)) return;

  const uint8_t manualMode = toByte(command.manualMode);
  const uint8_t overrideFillToHigh = toByte(command.overrideFillToHigh);
  const uint8_t emergencyStop = toByte(command.emergencyStop);
  const uint8_t autoPreferSump = toByte(command.autoPreferSump);
  const uint8_t forcedMotor = static_cast<uint8_t>(command.forcedMotor);
  const uint8_t checksum = checksumFor(
    manualMode,
    overrideFillToHigh,
    emergencyStop,
    autoPreferSump,
    forcedMotor
  );

  EEPROM.update(ADDR_MAGIC_0, MAGIC_0);
  EEPROM.update(ADDR_MAGIC_1, MAGIC_1);
  EEPROM.update(ADDR_VERSION, VERSION);
  EEPROM.update(ADDR_MANUAL_MODE, manualMode);
  EEPROM.update(ADDR_OVERRIDE, overrideFillToHigh);
  EEPROM.update(ADDR_ESTOP, emergencyStop);
  EEPROM.update(ADDR_PREFER_SUMP, autoPreferSump);
  EEPROM.update(ADDR_FORCED_MOTOR, forcedMotor);
  EEPROM.update(ADDR_CHECKSUM, checksum);

  lastSavedCommand = command;
  haveLastSavedCommand = true;
}
