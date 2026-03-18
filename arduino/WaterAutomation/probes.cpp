#include "probes.h"
#include <FspTimer.h>

namespace {

FspTimer probeTimer;
volatile bool txState = false;

void probeTimerISR(timer_callback_args_t* /* unused */) {
  txState = !txState;
  const uint8_t level = txState ? HIGH : LOW;
  digitalWrite(PIN_OH_TX, level);
  digitalWrite(PIN_SUMP_TX, level);
}

}

void initProbes() {

  pinMode(PIN_OH_TX, OUTPUT);
  digitalWrite(PIN_OH_TX, LOW);
  pinMode(PIN_SUMP_TX, OUTPUT);
  digitalWrite(PIN_SUMP_TX, LOW);

  pinMode(PIN_OH_LOW, INPUT);
  pinMode(PIN_OH_MED, INPUT);
  pinMode(PIN_OH_HIGH, INPUT);
  pinMode(PIN_SUMP_LOW, INPUT);
  pinMode(PIN_SUMP_HIGH, INPUT);

  uint8_t timerType = GPT_TIMER;
  int8_t timerIdx = FspTimer::get_available_timer(timerType);

  if (timerIdx < 0) {
    timerType = AGT_TIMER;
    timerIdx = FspTimer::get_available_timer(timerType);
  }

  if (timerIdx < 0) {
    Serial.println(F("probes: ERROR — no hardware timer available for TX ISR"));
    return;
  }

  FspTimer::force_use_of_pwm_reserved_timer();

  if (!probeTimer.begin(TIMER_MODE_PERIODIC, timerType, timerIdx,
                        AC_PROBE_FREQ_HZ, 0.0f, probeTimerISR)) {
    Serial.println(F("probes: ERROR — timer begin() failed"));
    return;
  }

  if (!probeTimer.setup_overflow_irq()) {
    Serial.println(F("probes: ERROR — setup_overflow_irq() failed"));
    return;
  }

  if (!probeTimer.open()) {
    Serial.println(F("probes: ERROR — timer open() failed"));
    return;
  }

  if (!probeTimer.start()) {
    Serial.println(F("probes: ERROR — timer start() failed"));
    return;
  }

  Serial.print(F("probes: TX ISR running at "));
  Serial.print(static_cast<int>(AC_PROBE_FREQ_HZ));
  Serial.println(F(" Hz"));

  delay(AC_PROBE_SETTLE_MS);
}

void stopProbes() {
  probeTimer.stop();
  digitalWrite(PIN_OH_TX, LOW);
  digitalWrite(PIN_SUMP_TX, LOW);
}

bool acProbeActive(uint8_t rxPin) {
  return digitalRead(rxPin) == HIGH;
}