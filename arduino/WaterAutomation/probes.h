#pragma once

#include "config.h"

void initProbes();
void stopProbes();

// Relies on the Option B smoothing circuit:
//   1N4148 diode + 100 nF capacitor + 100 kΩ pull-down on the RX pin.

// Arduino D5 (TX) ──[10kΩ]────────────────────────────────────────────── Probe A (in water)
//                                                                              │
//                                                                           (water)
//                                                                              │
// Arduino D7 (RX) ──[10kΩ]──[1N4148 diode]────────────────────────────── Probe B (in water)
//                                            |                     |
//                                    [100nF cap to GND]     [100kΩ to GND]

bool acProbeActive(uint8_t rxPin);