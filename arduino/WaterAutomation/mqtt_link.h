#pragma once

#include "controller.h"

void initMqttLink();
void runMqttLink(SystemState& state);
void publishStateToMqtt(const SystemState& state);
