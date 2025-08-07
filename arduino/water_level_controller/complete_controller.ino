/*
 * Water Level Automation System - Arduino Uno R4 WiFi
 * Complete Implementation with REST API
 * 
 * Hardware Requirements:
 * - Arduino Uno R4 WiFi
 * - 2x Water pumps with relay control
 * - 10x Float switches (5 per tank)
 * - 2x Current sensors (ACS712)
 * - 2-channel relay module
 * - RTC module (DS3231) - Optional
 * 
 * Pin Configuration:
 * Digital Pins 2-11: Float switches
 * Digital Pins 12-13: Pump relays
 * Analog Pins A0-A1: Current sensors
 * 
 * Author: Water Level Automation System
 * Version: 1.0.0
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// WiFi Configuration - UPDATE THESE VALUES
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Web Server
WebServer server(80);

// Pin Definitions
// Float Switches - Overhead Tank
#define OVERHEAD_EMPTY_PIN    2
#define OVERHEAD_LOW_PIN      3
#define OVERHEAD_MEDIUM_PIN   4
#define OVERHEAD_HIGH_PIN     5
#define OVERHEAD_FULL_PIN     6

// Float Switches - Secondary Tank
#define SECONDARY_EMPTY_PIN   7
#define SECONDARY_LOW_PIN     8
#define SECONDARY_MEDIUM_PIN  9
#define SECONDARY_HIGH_PIN    10
#define SECONDARY_FULL_PIN    11

// Pump Relays
#define PRIMARY_PUMP_RELAY    12
#define SECONDARY_PUMP_RELAY  13

// Current Sensors (Analog)
#define PRIMARY_CURRENT_PIN   A0
#define SECONDARY_CURRENT_PIN A1

// Status LED
#define STATUS_LED_PIN        LED_BUILTIN

// System Constants
#define CURRENT_SENSOR_SENSITIVITY 0.185  // ACS712-5A: 185mV/A
#define CURRENT_SENSOR_OFFSET 2.5          // 2.5V at 0A
#define MAX_RUNTIME_MINUTES 60             // Maximum continuous runtime
#define MAINTENANCE_HOURS 1000             // Maintenance interval
#define EEPROM_CONFIG_ADDR 0               // EEPROM configuration address
#define MAX_ALERTS 20                      // Maximum stored alerts

// System States
enum SystemMode {
  MODE_AUTO,
  MODE_MANUAL,
  MODE_SCHEDULED,
  MODE_EMERGENCY
};

enum PumpState {
  PUMP_OFF,
  PUMP_RUNNING,
  PUMP_FAULT,
  PUMP_DRY_RUN
};

enum WaterLevel {
  LEVEL_EMPTY = 0,
  LEVEL_LOW = 1,
  LEVEL_MEDIUM = 2,
  LEVEL_HIGH = 3,
  LEVEL_FULL = 4
};

// Alert Structure
struct Alert {
  String id;
  String type;
  String category;
  String title;
  String message;
  unsigned long timestamp;
  bool acknowledged;
};

// Configuration Structure
struct SystemConfig {
  char wifiSSID[32];
  char wifiPassword[32];
  float currentThresholdMin;
  float currentThresholdMax;
  int maxRuntimeMinutes;
  int maintenanceHours;
  bool autoMode;
  bool initialized;
};

// System Variables
SystemMode currentMode = MODE_AUTO;
PumpState primaryPumpState = PUMP_OFF;
PumpState secondaryPumpState = PUMP_OFF;

unsigned long primaryPumpStartTime = 0;
unsigned long secondaryPumpStartTime = 0;
unsigned long primaryTotalRuntime = 0;
unsigned long secondaryTotalRuntime = 0;
unsigned long primaryDailyRuntime = 0;
unsigned long secondaryDailyRuntime = 0;
unsigned long lastDayReset = 0;
unsigned long systemStartTime = 0;

bool emergencyStop = false;
bool primaryPumpActive = false;
bool secondaryPumpActive = false;

float primaryCurrent = 0.0;
float secondaryCurrent = 0.0;

SystemConfig config;
Alert activeAlerts[MAX_ALERTS];
int alertCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Water Level Automation System ===");
  Serial.println("Starting initialization...");
  
  // Initialize system timing
  systemStartTime = millis();
  lastDayReset = millis();
  
  // Initialize pins
  initializePins();
  
  // Load configuration
  loadConfiguration();
  
  // Initialize WiFi
  initializeWiFi();
  
  // Initialize web server
  initializeWebServer();
  
  // Initialize system
  initializeSystem();
  
  Serial.println("=== System Ready ===");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Web server running on port 80");
  Serial.println("========================");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Update sensor readings
  updateSensorReadings();
  
  // Update pump status
  updatePumpStatus();
  
  // Handle automatic mode logic
  if (currentMode == MODE_AUTO && !emergencyStop) {
    handleAutoMode();
  }
  
  // Handle scheduled mode logic
  if (currentMode == MODE_SCHEDULED && !emergencyStop) {
    handleScheduledMode();
  }
  
  // Safety checks
  performSafetyChecks();
  
  // Update daily runtime reset
  checkDailyReset();
  
  // Update status LED
  updateStatusLED();
  
  // Small delay to prevent overwhelming the system
  delay(100);
}

void initializePins() {
  Serial.println("Initializing pins...");
  
  // Float switches as inputs with pullup
  pinMode(OVERHEAD_EMPTY_PIN, INPUT_PULLUP);
  pinMode(OVERHEAD_LOW_PIN, INPUT_PULLUP);
  pinMode(OVERHEAD_MEDIUM_PIN, INPUT_PULLUP);
  pinMode(OVERHEAD_HIGH_PIN, INPUT_PULLUP);
  pinMode(OVERHEAD_FULL_PIN, INPUT_PULLUP);
  
  pinMode(SECONDARY_EMPTY_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_LOW_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_MEDIUM_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_HIGH_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_FULL_PIN, INPUT_PULLUP);
  
  // Pump relays as outputs (active LOW)
  pinMode(PRIMARY_PUMP_RELAY, OUTPUT);
  pinMode(SECONDARY_PUMP_RELAY, OUTPUT);
  digitalWrite(PRIMARY_PUMP_RELAY, HIGH);  // OFF
  digitalWrite(SECONDARY_PUMP_RELAY, HIGH); // OFF
  
  // Status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  Serial.println("Pins initialized successfully");
}

void loadConfiguration() {
  Serial.println("Loading configuration...");
  
  // Load configuration from EEPROM
  EEPROM.get(EEPROM_CONFIG_ADDR, config);
  
  // Check if configuration is valid
  if (!config.initialized || strlen(config.wifiSSID) == 0) {
    Serial.println("No valid configuration found, using defaults");
    
    // Set defaults
    strcpy(config.wifiSSID, ssid);
    strcpy(config.wifiPassword, password);
    config.currentThresholdMin = 0.5;
    config.currentThresholdMax = 8.0;
    config.maxRuntimeMinutes = MAX_RUNTIME_MINUTES;
    config.maintenanceHours = MAINTENANCE_HOURS;
    config.autoMode = true;
    config.initialized = true;
    
    saveConfiguration();
  }
  
  Serial.println("Configuration loaded successfully");
}

void saveConfiguration() {
  EEPROM.put(EEPROM_CONFIG_ADDR, config);
  Serial.println("Configuration saved to EEPROM");
}

void initializeWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(config.wifiSSID);
  
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    addAlert("critical", "wifi_error", "WiFi Error", "Failed to connect to WiFi network");
  }
}

void initializeWebServer() {
  Serial.println("Initializing web server...");
  
  // CORS headers for web compatibility
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(200);
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });
  
  // Status endpoints
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/tanks", HTTP_GET, handleGetTanks);
  server.on("/pumps", HTTP_GET, handleGetPumps);
  server.on("/alerts", HTTP_GET, handleGetAlerts);
  
  // Pump control endpoints
  server.on("/pump/start/primary", HTTP_POST, []() { handleStartPump("primary"); });
  server.on("/pump/start/secondary", HTTP_POST, []() { handleStartPump("secondary"); });
  server.on("/pump/stop", HTTP_POST, handleStopPump);
  server.on("/pump/switch", HTTP_POST, handleSwitchPump);
  server.on("/pump/test/primary", HTTP_POST, []() { handleTestPump("primary"); });
  server.on("/pump/test/secondary", HTTP_POST, []() { handleTestPump("secondary"); });
  
  // System mode endpoints
  server.on("/mode/auto", HTTP_POST, []() { handleSetMode("auto"); });
  server.on("/mode/manual", HTTP_POST, []() { handleSetMode("manual"); });
  server.on("/mode/scheduled", HTTP_POST, []() { handleSetMode("scheduled"); });
  server.on("/mode/emergency", HTTP_POST, []() { handleSetMode("emergency"); });
  
  // Configuration endpoints
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_POST, handleSetConfig);
  server.on("/reset", HTTP_POST, handleReset);
  
  server.begin();
  Serial.println("Web server initialized successfully");
}

void initializeSystem() {
  Serial.println("Initializing system...");
  
  emergencyStop = false;
  currentMode = MODE_AUTO;
  
  // Clear alerts
  alertCount = 0;
  
  // Add startup alert
  addAlert("info", "system_start", "System Started", "Water level automation system is now online");
  
  Serial.println("System initialized successfully");
}

void updateSensorReadings() {
  // Read current sensors
  primaryCurrent = readCurrent(PRIMARY_CURRENT_PIN);
  secondaryCurrent = readCurrent(SECONDARY_CURRENT_PIN);
}

float readCurrent(int pin) {
  // Take multiple readings for accuracy
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(1);
  }
  
  int sensorValue = sum / 10;
  float voltage = (sensorValue / 1024.0) * 5.0;
  float current = abs(voltage - CURRENT_SENSOR_OFFSET) / CURRENT_SENSOR_SENSITIVITY;
  
  return max(0.0, current); // Ensure non-negative current
}

WaterLevel readTankLevel(bool isOverhead) {
  int emptyPin, lowPin, mediumPin, highPin, fullPin;
  
  if (isOverhead) {
    emptyPin = OVERHEAD_EMPTY_PIN;
    lowPin = OVERHEAD_LOW_PIN;
    mediumPin = OVERHEAD_MEDIUM_PIN;
    highPin = OVERHEAD_HIGH_PIN;
    fullPin = OVERHEAD_FULL_PIN;
  } else {
    emptyPin = SECONDARY_EMPTY_PIN;
    lowPin = SECONDARY_LOW_PIN;
    mediumPin = SECONDARY_MEDIUM_PIN;
    highPin = SECONDARY_HIGH_PIN;
    fullPin = SECONDARY_FULL_PIN;
  }
  
  // Read float switches (LOW = water present due to pullup)
  bool empty = !digitalRead(emptyPin);
  bool low = !digitalRead(lowPin);
  bool medium = !digitalRead(mediumPin);
  bool high = !digitalRead(highPin);
  bool full = !digitalRead(fullPin);
  
  // Determine water level based on highest active sensor
  if (full) return LEVEL_FULL;
  if (high) return LEVEL_HIGH;
  if (medium) return LEVEL_MEDIUM;
  if (low) return LEVEL_LOW;
  return LEVEL_EMPTY;
}

void updatePumpStatus() {
  // Update primary pump status
  if (primaryPumpActive) {
    if (primaryCurrent < config.currentThresholdMin) {
      if (primaryPumpState != PUMP_DRY_RUN) {
        primaryPumpState = PUMP_DRY_RUN;
        addAlert("critical", "pump_dry_run", "Pump Dry Run", "Primary pump is running dry");
      }
    } else if (primaryCurrent > config.currentThresholdMax) {
      if (primaryPumpState != PUMP_FAULT) {
        primaryPumpState = PUMP_FAULT;
        addAlert("critical", "pump_fault", "Pump Fault", "Primary pump overcurrent detected");
      }
    } else {
      primaryPumpState = PUMP_RUNNING;
    }
    
    // Update runtime
    if (primaryPumpStartTime > 0) {
      unsigned long runtime = (millis() - primaryPumpStartTime) / 1000;
      primaryDailyRuntime += runtime;
      primaryTotalRuntime += runtime;
      primaryPumpStartTime = millis(); // Reset for next calculation
    }
  } else {
    primaryPumpState = PUMP_OFF;
  }
  
  // Update secondary pump status
  if (secondaryPumpActive) {
    if (secondaryCurrent < config.currentThresholdMin) {
      if (secondaryPumpState != PUMP_DRY_RUN) {
        secondaryPumpState = PUMP_DRY_RUN;
        addAlert("critical", "pump_dry_run", "Pump Dry Run", "Secondary pump is running dry");
      }
    } else if (secondaryCurrent > config.currentThresholdMax) {
      if (secondaryPumpState != PUMP_FAULT) {
        secondaryPumpState = PUMP_FAULT;
        addAlert("critical", "pump_fault", "Pump Fault", "Secondary pump overcurrent detected");
      }
    } else {
      secondaryPumpState = PUMP_RUNNING;
    }
    
    // Update runtime
    if (secondaryPumpStartTime > 0) {
      unsigned long runtime = (millis() - secondaryPumpStartTime) / 1000;
      secondaryDailyRuntime += runtime;
      secondaryTotalRuntime += runtime;
      secondaryPumpStartTime = millis(); // Reset for next calculation
    }
  } else {
    secondaryPumpState = PUMP_OFF;
  }
}

void handleAutoMode() {
  static unsigned long lastAutoCheck = 0;
  
  // Only check every 5 seconds to avoid rapid switching
  if (millis() - lastAutoCheck < 5000) return;
  lastAutoCheck = millis();
  
  WaterLevel overheadLevel = readTankLevel(true);
  WaterLevel secondaryLevel = readTankLevel(false);
  
  // Check if we need to start pumping
  if (overheadLevel <= LEVEL_LOW && !primaryPumpActive && !secondaryPumpActive) {
    if (secondaryLevel >= LEVEL_LOW) {
      startPump(true); // Start primary pump
      Serial.println("Auto mode: Starting primary pump - overhead tank low");
    } else {
      addAlert("critical", "tank_empty", "Tank Empty", "Secondary tank is empty - cannot pump water");
    }
  }
  
  // Check if we need to stop pumping
  if (overheadLevel >= LEVEL_FULL && (primaryPumpActive || secondaryPumpActive)) {
    stopAllPumps();
    addAlert("info", "tank_full", "Tank Full", "Overhead tank is full - pumps stopped");
    Serial.println("Auto mode: Stopping pumps - overhead tank full");
  }
  
  // Handle pump switching if primary fails
  if (primaryPumpActive && (primaryPumpState == PUMP_FAULT || primaryPumpState == PUMP_DRY_RUN)) {
    Serial.println("Auto mode: Primary pump fault detected, switching to secondary");
    stopPump(true);
    delay(2000); // Wait 2 seconds before switching
    
    if (secondaryLevel >= LEVEL_LOW) {
      startPump(false); // Switch to secondary
      addAlert("warning", "pump_switch", "Pump Switched", "Switched to secondary pump due to primary pump fault");
    }
  }
}

void handleScheduledMode() {
  // Placeholder for scheduled mode logic
  // For now, fall back to auto mode logic
  handleAutoMode();
}

void performSafetyChecks() {
  static unsigned long lastSafetyCheck = 0;
  
  // Perform safety checks every 10 seconds
  if (millis() - lastSafetyCheck < 10000) return;
  lastSafetyCheck = millis();
  
  // Check maximum runtime
  if (primaryPumpActive && (millis() - primaryPumpStartTime) > (config.maxRuntimeMinutes * 60000UL)) {
    stopPump(true);
    addAlert("warning", "max_runtime", "Max Runtime", "Primary pump stopped - maximum runtime exceeded");
    Serial.println("Safety: Primary pump stopped - max runtime exceeded");
  }
  
  if (secondaryPumpActive && (millis() - secondaryPumpStartTime) > (config.maxRuntimeMinutes * 60000UL)) {
    stopPump(false);
    addAlert("warning", "max_runtime", "Max Runtime", "Secondary pump stopped - maximum runtime exceeded");
    Serial.println("Safety: Secondary pump stopped - max runtime exceeded");
  }
  
  // Check for maintenance
  if (primaryTotalRuntime > (config.maintenanceHours * 3600UL)) {
    addAlert("warning", "maintenance_due", "Maintenance Due", "Primary pump maintenance is due");
  }
  
  if (secondaryTotalRuntime > (config.maintenanceHours * 3600UL)) {
    addAlert("warning", "maintenance_due", "Maintenance Due", "Secondary pump maintenance is due");
  }
  
  // Emergency stop check
  if (emergencyStop) {
    stopAllPumps();
  }
}

void checkDailyReset() {
  // Reset daily runtime at midnight (24 hours)
  if (millis() - lastDayReset > 86400000UL) {
    primaryDailyRuntime = 0;
    secondaryDailyRuntime = 0;
    lastDayReset = millis();
    Serial.println("Daily runtime counters reset");
  }
}

void updateStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (emergencyStop) {
    // Fast blink for emergency
    if (millis() - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
      lastBlink = millis();
    }
  } else if (primaryPumpActive || secondaryPumpActive) {
    // Solid on when pumping
    digitalWrite(STATUS_LED_PIN, HIGH);
  } else if (WiFi.status() != WL_CONNECTED) {
    // Slow blink for WiFi disconnected
    if (millis() - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
      lastBlink = millis();
    }
  } else {
    // Solid off when idle
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

void startPump(bool isPrimary) {
  if (emergencyStop) {
    Serial.println("Cannot start pump - emergency stop active");
    return;
  }
  
  if (isPrimary) {
    digitalWrite(PRIMARY_PUMP_RELAY, LOW); // Active LOW
    primaryPumpActive = true;
    primaryPumpStartTime = millis();
    Serial.println("Primary pump started");
  } else {
    digitalWrite(SECONDARY_PUMP_RELAY, LOW); // Active LOW
    secondaryPumpActive = true;
    secondaryPumpStartTime = millis();
    Serial.println("Secondary pump started");
  }
}

void stopPump(bool isPrimary) {
  if (isPrimary) {
    digitalWrite(PRIMARY_PUMP_RELAY, HIGH); // Active LOW
    primaryPumpActive = false;
    Serial.println("Primary pump stopped");
  } else {
    digitalWrite(SECONDARY_PUMP_RELAY, HIGH); // Active LOW
    secondaryPumpActive = false;
    Serial.println("Secondary pump stopped");
  }
}

void stopAllPumps() {
  stopPump(true);
  stopPump(false);
  Serial.println("All pumps stopped");
}

void addAlert(String type, String category, String title, String message) {
  if (alertCount >= MAX_ALERTS) {
    // Shift alerts to make room for new one
    for (int i = 0; i < MAX_ALERTS - 1; i++) {
      activeAlerts[i] = activeAlerts[i + 1];
    }
    alertCount = MAX_ALERTS - 1;
  }
  
  Alert newAlert;
  newAlert.id = String(millis());
  newAlert.type = type;
  newAlert.category = category;
  newAlert.title = title;
  newAlert.message = message;
  newAlert.timestamp = millis();
  newAlert.acknowledged = false;
  
  activeAlerts[alertCount] = newAlert;
  alertCount++;
  
  Serial.println("Alert: " + title + " - " + message);
}

// Web Server Handlers
void handleGetStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  DynamicJsonDocument doc(4096);
  
  // System status
  doc["mode"] = getModeString(currentMode);
  doc["active_pump"] = primaryPumpActive ? "primary" : (secondaryPumpActive ? "secondary" : "none");
  doc["connectivity"] = WiFi.status() == WL_CONNECTED;
  doc["uptime"] = (millis() - systemStartTime) / 1000;
  doc["alerts_count"] = alertCount;
  doc["emergency_stop"] = emergencyStop;
  
  // Tank levels
  WaterLevel overheadLevel = readTankLevel(true);
  WaterLevel secondaryLevel = readTankLevel(false);
  
  doc["overhead_level"] = (int)overheadLevel * 25; // Convert to percentage
  doc["sump_level"] = (int)secondaryLevel * 25;    // Convert to percentage
  doc["primary_pump"] = primaryPumpActive;
  doc["backup_pump"] = secondaryPumpActive;
  doc["auto_mode"] = (currentMode == MODE_AUTO);
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetTanks() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  DynamicJsonDocument doc(2048);
  JsonArray tanks = doc.createNestedArray("tanks");
  
  // Overhead tank
  JsonObject overheadTank = tanks.createNestedObject();
  overheadTank["tank_id"] = "overhead";
  WaterLevel overheadLevel = readTankLevel(true);
  overheadTank["level"] = getLevelString(overheadLevel);
  overheadTank["level_numeric"] = (int)overheadLevel;
  overheadTank["timestamp"] = millis();
  
  JsonObject overheadSensors = overheadTank.createNestedObject("sensors");
  overheadSensors["empty"] = !digitalRead(OVERHEAD_EMPTY_PIN);
  overheadSensors["low"] = !digitalRead(OVERHEAD_LOW_PIN);
  overheadSensors["medium"] = !digitalRead(OVERHEAD_MEDIUM_PIN);
  overheadSensors["high"] = !digitalRead(OVERHEAD_HIGH_PIN);
  overheadSensors["full"] = !digitalRead(OVERHEAD_FULL_PIN);
  
  // Secondary tank
  JsonObject secondaryTank = tanks.createNestedObject();
  secondaryTank["tank_id"] = "secondary";
  WaterLevel secondaryLevel = readTankLevel(false);
  secondaryTank["level"] = getLevelString(secondaryLevel);
  secondaryTank["level_numeric"] = (int)secondaryLevel;
  secondaryTank["timestamp"] = millis();
  
  JsonObject secondarySensors = secondaryTank.createNestedObject("sensors");
  secondarySensors["empty"] = !digitalRead(SECONDARY_EMPTY_PIN);
  secondarySensors["low"] = !digitalRead(SECONDARY_LOW_PIN);
  secondarySensors["medium"] = !digitalRead(SECONDARY_MEDIUM_PIN);
  secondarySensors["high"] = !digitalRead(SECONDARY_HIGH_PIN);
  secondarySensors["full"] = !digitalRead(SECONDARY_FULL_PIN);
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetPumps() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  DynamicJsonDocument doc(2048);
  JsonArray pumps = doc.createNestedArray("pumps");
  
  // Primary pump
  JsonObject primaryPump = pumps.createNestedObject();
  primaryPump["pump_id"] = "primary";
  primaryPump["status"] = getPumpStateString(primaryPumpState);
  primaryPump["current_draw"] = primaryCurrent;
  primaryPump["runtime_today"] = primaryDailyRuntime;
  primaryPump["total_runtime"] = primaryTotalRuntime;
  primaryPump["last_started"] = primaryPumpStartTime;
  primaryPump["maintenance_due"] = primaryTotalRuntime > (config.maintenanceHours * 3600UL);
  
  // Secondary pump
  JsonObject secondaryPump = pumps.createNestedObject();
  secondaryPump["pump_id"] = "secondary";
  secondaryPump["status"] = getPumpStateString(secondaryPumpState);
  secondaryPump["current_draw"] = secondaryCurrent;
  secondaryPump["runtime_today"] = secondaryDailyRuntime;
  secondaryPump["total_runtime"] = secondaryTotalRuntime;
  secondaryPump["last_started"] = secondaryPumpStartTime;
  secondaryPump["maintenance_due"] = secondaryTotalRuntime > (config.maintenanceHours * 3600UL);
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetAlerts() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  DynamicJsonDocument doc(4096);
  JsonArray alerts = doc.createNestedArray("alerts");
  
  for (int i = 0; i < alertCount; i++) {
    JsonObject alertObj = alerts.createNestedObject();
    alertObj["id"] = activeAlerts[i].id;
    alertObj["type"] = activeAlerts[i].type;
    alertObj["category"] = activeAlerts[i].category;
    alertObj["title"] = activeAlerts[i].title;
    alertObj["message"] = activeAlerts[i].message;
    alertObj["timestamp"] = activeAlerts[i].timestamp;
    alertObj["acknowledged"] = activeAlerts[i].acknowledged;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStartPump(String pumpId) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (emergencyStop) {
    server.send(400, "application/json", "{\"error\":\"Emergency stop active\"}");
    return;
  }
  
  if (pumpId == "primary") {
    startPump(true);
    currentMode = MODE_MANUAL;
    addAlert("info", "manual_start", "Manual Start", "Primary pump started manually");
  } else if (pumpId == "secondary") {
    startPump(false);
    currentMode = MODE_MANUAL;
    addAlert("info", "manual_start", "Manual Start", "Secondary pump started manually");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid pump ID\"}");
    return;
  }
  
  server.send(200, "application/json", "{\"success\":true}");
}

void handleStopPump() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  stopAllPumps();
  addAlert("info", "manual_stop", "Manual Stop", "All pumps stopped manually");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleSwitchPump() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (emergencyStop) {
    server.send(400, "application/json", "{\"error\":\"Emergency stop active\"}");
    return;
  }
  
  if (primaryPumpActive) {
    stopPump(true);
    delay(1000);
    startPump(false);
    addAlert("info", "manual_switch", "Manual Switch", "Switched from primary to secondary pump");
  } else if (secondaryPumpActive) {
    stopPump(false);
    delay(1000);
    startPump(true);
    addAlert("info", "manual_switch", "Manual Switch", "Switched from secondary to primary pump");
  } else {
    server.send(400, "application/json", "{\"error\":\"No pump currently running\"}");
    return;
  }
  
  server.send(200, "application/json