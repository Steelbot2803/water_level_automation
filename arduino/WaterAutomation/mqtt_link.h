#pragma once

#include "controller.h"

void initMqttLink();
void runMqttLink();
void publishStateToMqtt(const SystemState& state);
void checkMqttLiveness();
bool isWifiConnected();
bool isMqttConnected();