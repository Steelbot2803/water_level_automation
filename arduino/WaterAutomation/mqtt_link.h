#pragma once

#include "controller.h"

void initMqttLink();
void runMqttLink();
void publishHeartbeatToMqtt();
void publishStatusToMqtt(const SystemState& state, bool force);
void checkMqttLiveness();
bool isWifiConnected();
bool isMqttConnected();