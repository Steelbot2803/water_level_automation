#include "led_matrix.h"

#include <Arduino_LED_Matrix.h>
#include <stdio.h>
#include <string.h>

#include "mqtt_link.h"

namespace {

constexpr uint8_t MATRIX_WIDTH = 12;
constexpr uint8_t MATRIX_HEIGHT = 8;
constexpr uint8_t FONT_WIDTH = 3;
constexpr uint8_t FONT_HEIGHT = 5;
constexpr uint8_t FONT_SPACING = 1;
constexpr uint8_t TEXT_TOP = 1;
constexpr uint8_t MAX_SLOTS = 12;
constexpr uint8_t TEXT_MAX_LEN = 24;
constexpr unsigned long STATIC_HOLD_MS = 1500;
constexpr unsigned long SCROLL_STEP_MS = 90;
constexpr uint8_t SCROLL_PADDING = 4;

enum class DisplaySlotKind : uint8_t {
  TEXT,
  TANKS
};

struct DisplaySlot {
  DisplaySlotKind kind = DisplaySlotKind::TEXT;
  char text[TEXT_MAX_LEN] = {0};
};

struct LedMatrixRuntime {
  bool ready = false;
  uint8_t slotIndex = 0;
  unsigned long slotStartedAtMs = 0;
  unsigned long lastScrollStepMs = 0;
  uint16_t scrollOffset = 0;
};

ArduinoLEDMatrix matrix;
LedMatrixRuntime runtime;
uint8_t canvas[MATRIX_HEIGHT][MATRIX_WIDTH];

constexpr uint8_t GLYPH_SPACE[FONT_HEIGHT] = {0b000, 0b000, 0b000, 0b000, 0b000};
constexpr uint8_t GLYPH_DASH[FONT_HEIGHT] = {0b000, 0b000, 0b111, 0b000, 0b000};
constexpr uint8_t GLYPH_COLON[FONT_HEIGHT] = {0b000, 0b010, 0b000, 0b010, 0b000};
constexpr uint8_t GLYPH_0[FONT_HEIGHT] = {0b111, 0b101, 0b101, 0b101, 0b111};
constexpr uint8_t GLYPH_1[FONT_HEIGHT] = {0b010, 0b110, 0b010, 0b010, 0b111};
constexpr uint8_t GLYPH_2[FONT_HEIGHT] = {0b111, 0b001, 0b111, 0b100, 0b111};
constexpr uint8_t GLYPH_3[FONT_HEIGHT] = {0b111, 0b001, 0b111, 0b001, 0b111};
constexpr uint8_t GLYPH_4[FONT_HEIGHT] = {0b101, 0b101, 0b111, 0b001, 0b001};
constexpr uint8_t GLYPH_5[FONT_HEIGHT] = {0b111, 0b100, 0b111, 0b001, 0b111};
constexpr uint8_t GLYPH_6[FONT_HEIGHT] = {0b111, 0b100, 0b111, 0b101, 0b111};
constexpr uint8_t GLYPH_7[FONT_HEIGHT] = {0b111, 0b001, 0b001, 0b001, 0b001};
constexpr uint8_t GLYPH_8[FONT_HEIGHT] = {0b111, 0b101, 0b111, 0b101, 0b111};
constexpr uint8_t GLYPH_9[FONT_HEIGHT] = {0b111, 0b101, 0b111, 0b001, 0b111};
constexpr uint8_t GLYPH_A[FONT_HEIGHT] = {0b111, 0b101, 0b111, 0b101, 0b101};
constexpr uint8_t GLYPH_B[FONT_HEIGHT] = {0b110, 0b101, 0b110, 0b101, 0b110};
constexpr uint8_t GLYPH_C[FONT_HEIGHT] = {0b111, 0b100, 0b100, 0b100, 0b111};
constexpr uint8_t GLYPH_D[FONT_HEIGHT] = {0b110, 0b101, 0b101, 0b101, 0b110};
constexpr uint8_t GLYPH_E[FONT_HEIGHT] = {0b111, 0b100, 0b111, 0b100, 0b111};
constexpr uint8_t GLYPH_F[FONT_HEIGHT] = {0b111, 0b100, 0b111, 0b100, 0b100};
constexpr uint8_t GLYPH_G[FONT_HEIGHT] = {0b111, 0b100, 0b101, 0b101, 0b111};
constexpr uint8_t GLYPH_H[FONT_HEIGHT] = {0b101, 0b101, 0b111, 0b101, 0b101};
constexpr uint8_t GLYPH_I[FONT_HEIGHT] = {0b111, 0b010, 0b010, 0b010, 0b111};
constexpr uint8_t GLYPH_J[FONT_HEIGHT] = {0b111, 0b001, 0b001, 0b101, 0b111};
constexpr uint8_t GLYPH_K[FONT_HEIGHT] = {0b101, 0b101, 0b110, 0b101, 0b101};
constexpr uint8_t GLYPH_L[FONT_HEIGHT] = {0b100, 0b100, 0b100, 0b100, 0b111};
constexpr uint8_t GLYPH_M[FONT_HEIGHT] = {0b101, 0b111, 0b111, 0b101, 0b101};
constexpr uint8_t GLYPH_N[FONT_HEIGHT] = {0b101, 0b111, 0b111, 0b111, 0b101};
constexpr uint8_t GLYPH_O[FONT_HEIGHT] = {0b111, 0b101, 0b101, 0b101, 0b111};
constexpr uint8_t GLYPH_P[FONT_HEIGHT] = {0b111, 0b101, 0b111, 0b100, 0b100};
constexpr uint8_t GLYPH_Q[FONT_HEIGHT] = {0b111, 0b101, 0b101, 0b111, 0b001};
constexpr uint8_t GLYPH_R[FONT_HEIGHT] = {0b110, 0b101, 0b110, 0b101, 0b101};
constexpr uint8_t GLYPH_S[FONT_HEIGHT] = {0b111, 0b100, 0b111, 0b001, 0b111};
constexpr uint8_t GLYPH_T[FONT_HEIGHT] = {0b111, 0b010, 0b010, 0b010, 0b010};
constexpr uint8_t GLYPH_U[FONT_HEIGHT] = {0b101, 0b101, 0b101, 0b101, 0b111};
constexpr uint8_t GLYPH_V[FONT_HEIGHT] = {0b101, 0b101, 0b101, 0b101, 0b010};
constexpr uint8_t GLYPH_W[FONT_HEIGHT] = {0b101, 0b101, 0b111, 0b111, 0b101};
constexpr uint8_t GLYPH_X[FONT_HEIGHT] = {0b101, 0b101, 0b010, 0b101, 0b101};
constexpr uint8_t GLYPH_Y[FONT_HEIGHT] = {0b101, 0b101, 0b010, 0b010, 0b010};
constexpr uint8_t GLYPH_Z[FONT_HEIGHT] = {0b111, 0b001, 0b010, 0b100, 0b111};

const uint8_t* glyphFor(char c) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');

  switch (c) {
    case ' ': return GLYPH_SPACE;
    case '-': return GLYPH_DASH;
    case ':': return GLYPH_COLON;
    case '0': return GLYPH_0;
    case '1': return GLYPH_1;
    case '2': return GLYPH_2;
    case '3': return GLYPH_3;
    case '4': return GLYPH_4;
    case '5': return GLYPH_5;
    case '6': return GLYPH_6;
    case '7': return GLYPH_7;
    case '8': return GLYPH_8;
    case '9': return GLYPH_9;
    case 'A': return GLYPH_A;
    case 'B': return GLYPH_B;
    case 'C': return GLYPH_C;
    case 'D': return GLYPH_D;
    case 'E': return GLYPH_E;
    case 'F': return GLYPH_F;
    case 'G': return GLYPH_G;
    case 'H': return GLYPH_H;
    case 'I': return GLYPH_I;
    case 'J': return GLYPH_J;
    case 'K': return GLYPH_K;
    case 'L': return GLYPH_L;
    case 'M': return GLYPH_M;
    case 'N': return GLYPH_N;
    case 'O': return GLYPH_O;
    case 'P': return GLYPH_P;
    case 'Q': return GLYPH_Q;
    case 'R': return GLYPH_R;
    case 'S': return GLYPH_S;
    case 'T': return GLYPH_T;
    case 'U': return GLYPH_U;
    case 'V': return GLYPH_V;
    case 'W': return GLYPH_W;
    case 'X': return GLYPH_X;
    case 'Y': return GLYPH_Y;
    case 'Z': return GLYPH_Z;
    default: return GLYPH_SPACE;
  }
}

void clearCanvas() {
  memset(canvas, 0, sizeof(canvas));
}

void flushCanvas() {
  matrix.loadPixels(&canvas[0][0], MATRIX_WIDTH * MATRIX_HEIGHT);
}

void setPixel(int x, int y, bool on = true) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
  canvas[y][x] = on ? 1 : 0;
}

void drawRect(int x, int y, int width, int height) {
  for (int dx = 0; dx < width; ++dx) {
    setPixel(x + dx, y);
    setPixel(x + dx, y + height - 1);
  }
  for (int dy = 0; dy < height; ++dy) {
    setPixel(x, y + dy);
    setPixel(x + width - 1, y + dy);
  }
}

void fillRect(int x, int y, int width, int height) {
  for (int dy = 0; dy < height; ++dy) {
    for (int dx = 0; dx < width; ++dx) {
      setPixel(x + dx, y + dy);
    }
  }
}

uint8_t textWidth(const char* text) {
  if (text == nullptr || text[0] == '\0') return 0;
  const size_t len = strlen(text);
  return static_cast<uint8_t>((len * FONT_WIDTH) + ((len - 1) * FONT_SPACING));
}

void drawGlyph(char c, int originX, int originY) {
  const uint8_t* glyph = glyphFor(c);
  for (uint8_t row = 0; row < FONT_HEIGHT; ++row) {
    for (uint8_t col = 0; col < FONT_WIDTH; ++col) {
      if (glyph[row] & (1 << (FONT_WIDTH - 1 - col))) {
        setPixel(originX + col, originY + row);
      }
    }
  }
}

void drawText(const char* text, int originX, int originY) {
  if (text == nullptr) return;
  int cursorX = originX;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    drawGlyph(text[i], cursorX, originY);
    cursorX += FONT_WIDTH + FONT_SPACING;
  }
}

const char* levelShort(OverheadLevel level) {
  switch (level) {
    case OverheadLevel::CRITICAL: return "C";
    case OverheadLevel::LOW: return "L";
    case OverheadLevel::MEDIUM: return "M";
    case OverheadLevel::HIGH: return "H";
  }
  return "?";
}

const char* levelShort(SumpLevel level) {
  switch (level) {
    case SumpLevel::CRITICAL: return "C";
    case SumpLevel::LOW: return "L";
    case SumpLevel::HIGH: return "H";
  }
  return "?";
}

const char* motorShort(MotorType motor) {
  switch (motor) {
    case MotorType::BOREWELL: return "BW";
    case MotorType::SUMP: return "SU";
    case MotorType::NONE: return "IDLE";
  }
  return "?";
}

const char* statusShort(MotorStatus status) {
  switch (status) {
    case MotorStatus::STOPPED: return "STOP";
    case MotorStatus::STARTING: return "START";
    case MotorStatus::RUNNING: return "RUN";
    case MotorStatus::DRY_RUN_LOCK: return "LOCK";
    case MotorStatus::SUMP_CRITICAL: return "CRIT";
  }
  return "?";
}

const MotorRuntimeState& runtimeFor(const SystemState& state, MotorType motor) {
  return (motor == MotorType::SUMP) ? state.sump : state.borewell;
}

void addTextSlot(DisplaySlot* slots, size_t& count, const char* text) {
  if (count >= MAX_SLOTS) return;
  slots[count].kind = DisplaySlotKind::TEXT;
  snprintf(slots[count].text, sizeof(slots[count].text), "%s", text);
  ++count;
}

void addFormattedTextSlot(DisplaySlot* slots, size_t& count, const char* format, const char* valueA, const char* valueB = nullptr) {
  if (count >= MAX_SLOTS) return;
  slots[count].kind = DisplaySlotKind::TEXT;
  if (valueB == nullptr) snprintf(slots[count].text, sizeof(slots[count].text), format, valueA);
  else snprintf(slots[count].text, sizeof(slots[count].text), format, valueA, valueB);
  ++count;
}

void addTanksSlot(DisplaySlot* slots, size_t& count) {
  if (count >= MAX_SLOTS) return;
  slots[count].kind = DisplaySlotKind::TANKS;
  slots[count].text[0] = '\0';
  ++count;
}

size_t buildSlots(const SystemState& state, DisplaySlot* slots) {
  size_t count = 0;

  if (state.command.emergencyStop) addTextSlot(slots, count, "ESTOP");
  if (state.command.overrideFillToHigh) addTextSlot(slots, count, "OVERRIDE");
  if (state.overheadLevel == OverheadLevel::CRITICAL) addTextSlot(slots, count, "OH CRIT");
  if (state.sumpLevel == SumpLevel::CRITICAL || state.sumpCriticalWarningLatched) addTextSlot(slots, count, "SUMP CRIT");
  if (state.borewell.status == MotorStatus::DRY_RUN_LOCK) addTextSlot(slots, count, "BW LOCK");
  if (state.sump.status == MotorStatus::DRY_RUN_LOCK) addTextSlot(slots, count, "SU LOCK");
  if (state.sump.status == MotorStatus::SUMP_CRITICAL) addTextSlot(slots, count, "SU BLOCK");
  if (!isWifiConnected()) addTextSlot(slots, count, "WIFI OFF");
  if (!isMqttConnected()) addTextSlot(slots, count, "MQTT OFF");

  addFormattedTextSlot(slots, count, "MODE %s", state.command.manualMode ? "MAN" : "AUTO");
  addFormattedTextSlot(slots, count, "PREF %s", state.command.autoPreferSump ? "SU" : "BW");

  if (state.activeMotor == MotorType::NONE) {
    addTextSlot(slots, count, "PUMP IDLE");
  } else {
    const char* motor = motorShort(state.activeMotor);
    const char* status = statusShort(runtimeFor(state, state.activeMotor).status);
    addFormattedTextSlot(slots, count, "PUMP %s %s", motor, status);
  }

  addFormattedTextSlot(slots, count, "OH %s SU %s", levelShort(state.overheadLevel), levelShort(state.sumpLevel));
  addTanksSlot(slots, count);

  return count;
}

void resetSlotProgress(unsigned long nowMs) {
  runtime.slotStartedAtMs = nowMs;
  runtime.lastScrollStepMs = nowMs;
  runtime.scrollOffset = 0;
}

void advanceSlot(size_t slotCount, unsigned long nowMs) {
  runtime.slotIndex = static_cast<uint8_t>((runtime.slotIndex + 1) % slotCount);
  resetSlotProgress(nowMs);
}

void renderTextSlot(const char* text, unsigned long nowMs, size_t slotCount) {
  clearCanvas();
  const uint8_t width = textWidth(text);

  if (width <= MATRIX_WIDTH) {
    const int centeredX = (MATRIX_WIDTH - width) / 2;
    drawText(text, centeredX, TEXT_TOP);
    flushCanvas();

    if (nowMs - runtime.slotStartedAtMs >= STATIC_HOLD_MS) {
      advanceSlot(slotCount, nowMs);
    }
    return;
  }

  if (nowMs - runtime.lastScrollStepMs >= SCROLL_STEP_MS) {
    runtime.lastScrollStepMs = nowMs;
    ++runtime.scrollOffset;
  }

  const int originX = static_cast<int>(SCROLL_PADDING) - static_cast<int>(runtime.scrollOffset);
  drawText(text, originX, TEXT_TOP);
  flushCanvas();

  const uint16_t travel = static_cast<uint16_t>(width + (SCROLL_PADDING * 2) - MATRIX_WIDTH);
  if (runtime.scrollOffset > travel) {
    advanceSlot(slotCount, nowMs);
  }
}

uint8_t overheadFillRows(OverheadLevel level) {
  switch (level) {
    case OverheadLevel::CRITICAL: return 0;
    case OverheadLevel::LOW: return 1;
    case OverheadLevel::MEDIUM: return 2;
    case OverheadLevel::HIGH: return 4;
  }
  return 0;
}

uint8_t sumpFillRows(SumpLevel level) {
  switch (level) {
    case SumpLevel::CRITICAL: return 0;
    case SumpLevel::LOW: return 2;
    case SumpLevel::HIGH: return 4;
  }
  return 0;
}

void drawTank(int x, int fillRows, bool alert) {
  drawRect(x, 1, 4, 6);
  setPixel(x + 1, 0);
  setPixel(x + 2, 0);

  for (int row = 0; row < fillRows; ++row) {
    fillRect(x + 1, 5 - row, 2, 1);
  }

  if (alert) {
    setPixel(x + 1, 2);
    setPixel(x + 2, 2);
    setPixel(x + 1, 4);
    setPixel(x + 2, 4);
  }
}

void renderTankSlot(const SystemState& state, unsigned long nowMs, size_t slotCount) {
  clearCanvas();
  drawTank(1, overheadFillRows(state.overheadLevel), state.overheadLevel == OverheadLevel::CRITICAL);
  drawTank(7, sumpFillRows(state.sumpLevel), state.sumpLevel == SumpLevel::CRITICAL);

  if (state.activeMotor == MotorType::BOREWELL) {
    setPixel(0, 7);
    setPixel(1, 7);
  } else if (state.activeMotor == MotorType::SUMP) {
    setPixel(10, 7);
    setPixel(11, 7);
  }

  flushCanvas();

  if (nowMs - runtime.slotStartedAtMs >= STATIC_HOLD_MS) {
    advanceSlot(slotCount, nowMs);
  }
}

}

void initLedMatrix() {
  runtime.ready = matrix.begin();
  runtime.slotIndex = 0;
  resetSlotProgress(millis());

  if (!runtime.ready) {
    Serial.println(F("led_matrix: begin() failed"));
    return;
  }

  Serial.println(F("led_matrix: ready"));
  matrix.clear();
}

void updateLedMatrix(const SystemState& state) {
  if (!runtime.ready) return;

  DisplaySlot slots[MAX_SLOTS];
  const size_t slotCount = buildSlots(state, slots);
  if (slotCount == 0) {
    matrix.clear();
    return;
  }

  if (runtime.slotIndex >= slotCount) {
    runtime.slotIndex = 0;
    resetSlotProgress(millis());
  }

  const unsigned long nowMs = millis();
  const DisplaySlot& slot = slots[runtime.slotIndex];

  if (slot.kind == DisplaySlotKind::TANKS) {
    renderTankSlot(state, nowMs, slotCount);
    return;
  }

  renderTextSlot(slot.text, nowMs, slotCount);
}
