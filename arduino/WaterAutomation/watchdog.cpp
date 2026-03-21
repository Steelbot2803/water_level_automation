#include "watchdog.h"

#include <Arduino.h>
#include <WDT.h>

namespace {

bool initialised = false;

}

bool watchdogBegin() {
  if (!WDT.begin(5000)) {
    Serial.println(F("watchdog: WDT.begin() failed"));
    return false;
  }

  initialised = true;
  WDT.refresh();
  Serial.println(F("watchdog: running (HW WDT 5 s, refreshed by main loop)"));
  return true;
}

void watchdogKick() {
  if (!initialised) return;
  WDT.refresh();
}
