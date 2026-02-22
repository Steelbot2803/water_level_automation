#pragma once

#include "Config.h"
#include "Types.h"

class Sensors {
 public:
  void begin();
  SensorSnapshot read() const;

 private:
  static bool isWet(uint8_t pin);
};
