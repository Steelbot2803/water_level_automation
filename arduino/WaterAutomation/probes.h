#pragma once

#include "config.h"

void initProbes();
void stopProbes();

// Relies on the Option B smoothing circuit:
//   1N4148 diode + 100 nF capacitor + 100 kΩ pull-down on the RX pin.
bool acProbeActive(uint8_t rxPin);