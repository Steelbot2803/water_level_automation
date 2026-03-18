#include "watchdog.h"

#include <Arduino.h>
#include <FspTimer.h>
#include <WDT.h>

// ---------------------------------------------------------------------------
// How this works
// ---------------------------------------------------------------------------
//
// The RA4M1 hardware WDT has a maximum timeout of ~5592 ms. That is shorter
// than this app's WiFi retry window (10 s) and a slow MQTT connect. If we
// kicked the WDT only from loop(), a perfectly normal reconnect would trigger
// a reset. To avoid that, we kick the WDT from a hardware timer ISR that
// fires every second — completely independently of loop().
//
// The ISR also checks how long it has been since loop() last called
// watchdogKick(). If that gap exceeds WATCHDOG_LOOP_TIMEOUT_MS the ISR calls
// NVIC_SystemReset() directly. This catches a frozen-but-not-crashed loop.
//
// Safety note on relays:
//   Both relay outputs are active-HIGH (see config.h). On reset, all GPIO
//   pins revert to INPUT (high-impedance) in the RA4M1 bootloader before
//   setup() runs. A high-impedance pin on an active-HIGH relay driver means
//   the relay is OFF — the safe state. No extra circuitry is needed.
// ---------------------------------------------------------------------------

namespace {

FspTimer wdtTimer;
volatile unsigned long lastLoopKickMs = 0;
bool initialised = false;

void timerCallback(timer_callback_args_t* /* unused */) {

  WDT.refresh();

  if (initialised) {
    const unsigned long now = millis();
    const unsigned long gap = now - lastLoopKickMs;
    if (gap > WATCHDOG_LOOP_TIMEOUT_MS) {
      NVIC_SystemReset();
    }
  }
}

bool startBackgroundTimer(float rateHz) {
  uint8_t timerType = GPT_TIMER;
  int8_t timerIdx = FspTimer::get_available_timer(timerType);

  if (timerIdx < 0) {
    timerType = AGT_TIMER;
    timerIdx = FspTimer::get_available_timer(timerType);
  }
  if (timerIdx < 0) return false;

  FspTimer::force_use_of_pwm_reserved_timer();

  if (!wdtTimer.begin(TIMER_MODE_PERIODIC, timerType, timerIdx,
                      rateHz, 0.0f, timerCallback)) return false;
  if (!wdtTimer.setup_overflow_irq()) return false;
  if (!wdtTimer.open()) return false;
  if (!wdtTimer.start()) return false;

  return true;
}

}

bool watchdogBegin() {

  lastLoopKickMs = millis();

  if (!WDT.begin(5000)) {
    Serial.println(F("watchdog: WDT.begin() failed"));
    return false;
  }
  WDT.refresh();

  if (!startBackgroundTimer(1.0f)) {
    Serial.println(F("watchdog: no hardware timer available"));
    return false;
  }

  initialised = true;
  Serial.println(F("watchdog: running (HW WDT 5 s, loop timeout 5 min)"));
  return true;
}

void watchdogKick() {
  lastLoopKickMs = millis();
}