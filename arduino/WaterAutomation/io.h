#pragma once

#include "controller.h"

void readCommandFromSerial(SystemState& state);
void printBanner();

// Applies a single normalized command string (same grammar as serial input).
bool applyCommand(SystemState& state, const String& line);

// Global state hook for MQTT callbacks.
void setSystemStateRef(SystemState* state);
SystemState* getSystemStateRef();
