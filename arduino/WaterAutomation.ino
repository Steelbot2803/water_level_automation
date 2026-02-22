#include "src/SystemController.h"

SystemController controller;

void setup() {
  controller.begin();
}

void loop() {
  controller.loop();
}
