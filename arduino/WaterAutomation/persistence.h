#pragma once

#include "controller.h"

void initPersistence();
void loadPersistedState(SystemState& state);
void persistStateIfChanged(const SystemState& state);
