#pragma once

#include "controller.h"

void initMqttLink();
void runMqttLink();
void publishStateToMqtt(const SystemState& state);

bool isWifiConnected();
bool isMqttConnected();