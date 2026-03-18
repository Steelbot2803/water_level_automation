#pragma once

constexpr unsigned long WATCHDOG_LOOP_TIMEOUT_MS = 5UL * 60UL * 1000UL;  // 5 min

bool watchdogBegin();

void watchdogKick();