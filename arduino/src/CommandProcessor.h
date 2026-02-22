#pragma once

#include "Types.h"

class CommandProcessor {
 public:
  void begin(unsigned long baud = 115200);
  void poll(UserCommand& command);
  void printHelp() const;

 private:
  String line_;
  void applyLine(const String& line, UserCommand& command);
};
