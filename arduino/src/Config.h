#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t OH_CRITICAL = 2;
constexpr uint8_t OH_LOW = 3;
constexpr uint8_t OH_MED = 4;
constexpr uint8_t OH_HIGH = 5;

constexpr uint8_t SUMP_CRITICAL = 6;
constexpr uint8_t SUMP_LOW = 7;
constexpr uint8_t SUMP_HIGH = 8;

constexpr uint8_t BOREWELL_DRY_RUN = 9;
constexpr uint8_t SUMP_DRY_RUN = 10;

constexpr uint8_t BOREWELL_RELAY = 11;
constexpr uint8_t SUMP_RELAY = 12;
}  // namespace Pins

namespace Timing {
constexpr unsigned long STATUS_PRINT_INTERVAL_MS = 2000;
}
