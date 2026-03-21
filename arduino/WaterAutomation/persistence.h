#pragma once

#include "controller.h"

void initPersistence();
void loadPersistedCommandState(CommandState& command);
void persistCommandStateIfChanged(const CommandState& command);
