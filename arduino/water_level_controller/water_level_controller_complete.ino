/*
 * Water Level Automation System - Complete Arduino Controller
 * Compatible with Arduino Uno R4 WiFi
 * 
 * Features:
 * - Dual pump system with automatic failover
 * - Float switch water level monitoring (5 levels per tank)
 * - Current monitoring for pump fault detection
 * - REST API for Flutter app integration
 * - Manual override with safety controls
 * - Emergency stop functionality
 * - Configuration persistence in EEPROM
 * - Real-time alerts and notifications
 * 
 * Hardware Requirements:
 * - Arduino Uno R4 WiFi
 * - 2x Relay modules (for pump control)
 * - 10x Float switches (5 per tank)
 * - 2x ACS712 current sensors
 * - Pull-up resistors for float switches
 */

#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Enums
enum WaterLevel {
  LEVEL_EMPTY,
  LEVEL_LOW,
  LEVEL_MEDIUM,
  LEVEL_HIGH,
  LEVEL_FULL
};

enum PumpState {
  PUMP_OFF,
  PUMP_RUNNING,
  PUMP_FAULT,
  PUMP_DRY_RUN
};

enum SystemMode {
  MODE_AUTO,
  MODE_MANUAL,
  MODE_SCHEDULED,
  MODE_EMERGENCY
};

// Function declarations
void addAlert(const char* type, const char* category, const char* title, const char* message);
void updatePumpStates();
void sendStatusResponse();
void sendTanksResponse();
void sendPumpsResponse();
void sendAlertsResponse();
void sendConfigResponse();
void sendSimpleResponse(const char* json, int code = 200);
void stopPump(bool isPrimary);
void stopAllPumps();
void startPump(bool isPrimary);
void updateSystemStatus();
void automaticControl();
void scheduledControl();
void emergencyControl();
void handleSerialCommands();
void checkMaintenance();
void handleReset();
void printSystemStatus();
void debugSensors();
String getModeString(SystemMode mode);
String getPumpStateString(PumpState state);
String getLevelString(WaterLevel level);

// Pin Definitions
// Overhead Tank Float Switches
#define OVERHEAD_EMPTY_PIN    2
#define OVERHEAD_LOW_PIN      3
#define OVERHEAD_MEDIUM_PIN   4
#define OVERHEAD_HIGH_PIN     5
#define OVERHEAD_FULL_PIN     6

// Secondary Tank Float Switches
#define SECONDARY_EMPTY_PIN   7
#define SECONDARY_LOW_PIN     8
#define SECONDARY_MEDIUM_PIN  9
#define SECONDARY_HIGH_PIN    10
#define SECONDARY_FULL_PIN    11

// Pump Control Relays (Active LOW)
#define PRIMARY_PUMP_RELAY    12
#define SECONDARY_PUMP_RELAY  13

// Current Sensors (Analog)
#define PRIMARY_CURRENT_PIN   A0
#define SECONDARY_CURRENT_PIN A1

// System Constants
#define CURRENT_SENSOR_SENSITIVITY 0.185  // ACS712-5A: 185mV/A
#define CURRENT_SENSOR_OFFSET 2.5          // 2.5V at 0A
#define MAX_RUNTIME_MINUTES 60             // Maximum continuous runtime
#define MAINTENANCE_HOURS 100              // Hours between maintenance
#define EEPROM_CONFIG_ADDRESS 0            // EEPROM address for config
#define MAX_ALERTS 20                      // Maximum stored alerts

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Web Server
WiFiServer server(80);
WiFiClient client;

// Enums
enum WaterLevel {
  LEVEL_EMPTY,
  LEVEL_LOW,
  LEVEL_MEDIUM,
  LEVEL_HIGH,
  LEVEL_FULL
};

enum PumpState {
  PUMP_OFF,
  PUMP_RUNNING,
  PUMP_FAULT,
  PUMP_DRY_RUN
};

enum SystemMode {
  MODE_AUTO,
  MODE_MANUAL,
  MODE_SCHEDULED,
  MODE_EMERGENCY
};

// Configuration Structure
struct SystemConfig {
  char wifiSSID[32];
  char wifiPassword[64];
  float currentThresholdMin;
  float currentThresholdMax;
  int maxRuntimeMinutes;
  int maintenanceHours;
  bool autoMode;
};

// Alert Structure
struct Alert {
  char type[16];        // "info", "warning", "critical"
  char category[32];    // "pump_fault", "water_level", etc.
  char title[64];
  char message[128];
  unsigned long timestamp;
};

// Global Variables
SystemConfig config;
SystemMode currentMode = MODE_AUTO;
PumpState primaryPumpState = PUMP_OFF;
PumpState secondaryPumpState = PUMP_OFF;
bool emergencyStop = false;
bool primaryPumpActive = false;
bool secondaryPumpActive = false;

// Runtime tracking
unsigned long primaryStartTime = 0;
unsigned long secondaryStartTime = 0;
unsigned long primaryTotalRuntime = 0;
unsigned long secondaryTotalRuntime = 0;
unsigned long primaryDailyRuntime = 0;
unsigned long secondaryDailyRuntime = 0;
unsigned long lastDayReset = 0;

// Current monitoring
float primaryCurrent = 0.0;
float secondaryCurrent = 0.0;

// Alert system
Alert alerts[MAX_ALERTS];
int alertCount = 0;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastMaintenanceCheck = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Water Level Controller Starting...");
  
  // Initialize pins
  initializePins();
  
  // Load configuration
  loadConfiguration();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  // Initialize system
  currentMode = MODE_AUTO;
  emergencyStop = false;
  lastDayReset = millis();
  
  Serial.println("System initialized successfully");
  Serial.println("Available commands: status, debug, start1, start2, stop, auto, manual, emergency, reset, help");
}

// ...existing code...

void initializePins() {
  // Float switch pins (INPUT_PULLUP for normally open switches)
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
  
  // Relay pins (OUTPUT, start HIGH for active-low relays)
  pinMode(PRIMARY_PUMP_RELAY, OUTPUT);
  pinMode(SECONDARY_PUMP_RELAY, OUTPUT);
  digitalWrite(PRIMARY_PUMP_RELAY, HIGH);    // OFF
  digitalWrite(SECONDARY_PUMP_RELAY, HIGH);  // OFF
  
  Serial.println("Pins initialized");
}

void loadConfiguration() {
  EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
  // Check if configuration is valid (simple validation)
  if (config.currentThresholdMin < 0 || config.currentThresholdMin > 10) {
    // Load defaults
    strcpy(config.wifiSSID, ssid);
    strcpy(config.wifiPassword, password);
    config.currentThresholdMin = 0.5;
    config.currentThresholdMax = 8.0;
    config.maxRuntimeMinutes = MAX_RUNTIME_MINUTES;
    config.maintenanceHours = MAINTENANCE_HOURS;
    config.autoMode = true;
    saveConfiguration();
    Serial.println("Default configuration loaded");
  } else {
    Serial.println("Configuration loaded from EEPROM");
  }
}

void saveConfiguration() {
  EEPROM.put(EEPROM_CONFIG_ADDRESS, config);
  Serial.println("Configuration saved to EEPROM");
}

void connectToWiFi() {
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    addAlert("info", "wifi_connected", "WiFi Connected", "System connected to WiFi network");
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    addAlert("warning", "wifi_failed", "WiFi Failed", "Failed to connect to WiFi network");
  }
}

void setupWebServer() {
  server.begin();
  Serial.println("Web server started on port 80");
}

void readSensors() {
  // Read current sensors
  int primaryRaw = analogRead(PRIMARY_CURRENT_PIN);
  int secondaryRaw = analogRead(SECONDARY_CURRENT_PIN);
  // Convert to voltage (Arduino Uno R4 WiFi: 5V reference, 10-bit ADC)
  float primaryVoltage = (primaryRaw * 5.0) / 1023.0;
  float secondaryVoltage = (secondaryRaw * 5.0) / 1023.0;
  // Convert to current
  primaryCurrent = abs(primaryVoltage - CURRENT_SENSOR_OFFSET) / CURRENT_SENSOR_SENSITIVITY;
  secondaryCurrent = abs(secondaryVoltage - CURRENT_SENSOR_OFFSET) / CURRENT_SENSOR_SENSITIVITY;
  // Update pump states based on current readings
  updatePumpStates();
}
void loop() {
  // Handle HTTP requests
  client = server.available();
  if (client) {
    String req = "";
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        req += c;
        if (c == '\n' && currentLineIsBlank) {
          // Parse HTTP request
          if (req.indexOf("GET /status") >= 0) {
            sendStatusResponse();
          } else if (req.indexOf("GET /tanks") >= 0) {
            sendTanksResponse();
          } else if (req.indexOf("GET /pumps") >= 0) {
            sendPumpsResponse();
          } else if (req.indexOf("GET /alerts") >= 0) {
            sendAlertsResponse();
          } else if (req.indexOf("GET /config") >= 0) {
            sendConfigResponse();
          } else if (req.indexOf("POST /pump/start") >= 0) {
            // For demo, always start primary
            startPump(true);
            sendSimpleResponse("{\"success\":true}");
          } else if (req.indexOf("POST /pump/stop") >= 0) {
            stopAllPumps();
            sendSimpleResponse("{\"success\":true}");
          } else if (req.indexOf("POST /mode") >= 0) {
            currentMode = MODE_AUTO;
            sendSimpleResponse("{\"success\":true}");
          } else if (req.indexOf("POST /reset") >= 0) {
            handleReset();
            sendSimpleResponse("{\"success\":true}");
          } else if (req.indexOf("POST /emergency") >= 0) {
            emergencyStop = true;
            stopAllPumps();
            sendSimpleResponse("{\"success\":true}");
          } else {
            sendSimpleResponse("{\"error\":\"Unknown endpoint\"}", 404);
          }
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }

  // Handle serial commands for debugging
  handleSerialCommands();

  // Read sensors every 1 second
  if (millis() - lastSensorRead > 1000) {
    readSensors();
    lastSensorRead = millis();
  }

  // Update system status every 5 seconds
  if (millis() - lastStatusUpdate > 5000) {
    updateSystemStatus();
    lastStatusUpdate = millis();
  }

  // Check maintenance every hour
  if (millis() - lastMaintenanceCheck > 3600000) {
    checkMaintenance();
    lastMaintenanceCheck = millis();
  }

  // Reset daily counters at midnight (24 hours)
  if (millis() - lastDayReset > 86400000) {
    primaryDailyRuntime = 0;
    secondaryDailyRuntime = 0;
    lastDayReset = millis();
    Serial.println("Daily runtime counters reset");
  }

  // Main control logic
  if (!emergencyStop) {
    switch (currentMode) {
      case MODE_AUTO:
        automaticControl();
        break;
      case MODE_MANUAL:
        // Manual mode - pumps controlled via API only
        break;
      case MODE_SCHEDULED:
        scheduledControl();
        break;
      case MODE_EMERGENCY:
        emergencyControl();
        break;
    }
  }

  // Runtime tracking for primary pump
  unsigned long currentTime = millis();  // Get current time
  if (primaryPumpActive && primaryStartTime > 0) {
    unsigned long runtime = (currentTime - primaryStartTime) / 1000; // seconds
    primaryDailyRuntime += runtime;
    primaryTotalRuntime += runtime;
    primaryStartTime = currentTime; // Reset for next calculation
    
    // Check maximum runtime
    if (runtime > (config.maxRuntimeMinutes * 60)) {
      addAlert("warning", "max_runtime", "Maximum Runtime", "Primary pump exceeded maximum runtime - stopped");
      stopPump(true);
    }
  }
  
  // Runtime tracking for secondary pump
  if (secondaryPumpActive && secondaryStartTime > 0) {
    unsigned long runtime = (currentTime - secondaryStartTime) / 1000; // seconds
    secondaryDailyRuntime += runtime;
    secondaryTotalRuntime += runtime;
    secondaryStartTime = currentTime; // Reset for next calculation
    
    // Check maximum runtime
    if (runtime > (config.maxRuntimeMinutes * 60)) {
      addAlert("warning", "max_runtime", "Maximum Runtime", "Secondary pump exceeded maximum runtime - stopped");
      stopPump(false);
    }
  }

  // Small delay to prevent overwhelming the system
  delay(10);
}

void checkMaintenance() {
  // Check if pumps need maintenance based on total runtime
  unsigned long maintenanceThreshold = config.maintenanceHours * 3600; // Convert to seconds
  
  if (primaryTotalRuntime > maintenanceThreshold) {
    addAlert("info", "maintenance_due", "Maintenance Due", "Primary pump maintenance is due");
  }
  
  if (secondaryTotalRuntime > maintenanceThreshold) {
    addAlert("info", "maintenance_due", "Maintenance Due", "Secondary pump maintenance is due");
  }
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
  
  // Read float switches (active LOW due to pullup resistors)
  bool empty = !digitalRead(emptyPin);
  bool low = !digitalRead(lowPin);
  bool medium = !digitalRead(mediumPin);
  bool high = !digitalRead(highPin);
  bool full = !digitalRead(fullPin);
  
  // Determine water level based on active switches
  if (full) return LEVEL_FULL;
  if (high) return LEVEL_HIGH;
  if (medium) return LEVEL_MEDIUM;
  if (low) return LEVEL_LOW;
  if (empty) return LEVEL_EMPTY;
  
  // Default to empty if no switches are active
  return LEVEL_EMPTY;
}

void automaticControl() {
  WaterLevel overheadLevel = readTankLevel(true);
  WaterLevel secondaryLevel = readTankLevel(false);
  
  // Auto control logic
  bool shouldPumpRun = false;
  bool usePrimary = true;
  
  // Determine if pumping is needed
  if (overheadLevel == LEVEL_EMPTY || overheadLevel == LEVEL_LOW) {
    shouldPumpRun = true;
  }
  
  // Stop pumping if tank is full
  if (overheadLevel == LEVEL_FULL) {
    shouldPumpRun = false;
  }
  
  // Determine which pump to use
  if (primaryPumpState == PUMP_FAULT || primaryPumpState == PUMP_DRY_RUN) {
    usePrimary = false; // Use secondary pump
  }
  
  // Check secondary tank level for source availability
  if (secondaryLevel == LEVEL_EMPTY) {
    shouldPumpRun = false;
    if (primaryPumpActive || secondaryPumpActive) {
      addAlert("warning", "no_source_water", "No Source Water", "Secondary tank is empty - pumps stopped");
    }
  }
  
  // Execute pump control
  if (shouldPumpRun) {
    if (usePrimary && !primaryPumpActive) {
      startPump(true);
    } else if (!usePrimary && !secondaryPumpActive) {
      startPump(false);
    }
  } else {
    if (primaryPumpActive) stopPump(true);
    if (secondaryPumpActive) stopPump(false);
  }
}

void scheduledControl() {
  // Placeholder for scheduled control logic
  // This would implement time-based pump scheduling
  automaticControl(); // Fall back to automatic for now
}

void emergencyControl() {
  // Emergency mode - stop all pumps
  stopAllPumps();
void addAlert(const char* type, const char* category, const char* title, const char* message) {
  if (alertCount < MAX_ALERTS) {
    alerts[alertCount].timestamp = millis();
    strncpy(alerts[alertCount].type, type, sizeof(alerts[alertCount].type) - 1);
    strncpy(alerts[alertCount].category, category, sizeof(alerts[alertCount].category) - 1);
    strncpy(alerts[alertCount].title, title, sizeof(alerts[alertCount].title) - 1);
    strncpy(alerts[alertCount].message, message, sizeof(alerts[alertCount].message) - 1);
    alertCount++;
  }
  // Also print to Serial for debugging
  Serial.print("[ALERT] "); Serial.print(type); Serial.print(": "); Serial.println(message);
}

void updatePumpStates() {
  // Update pump states based on current readings
  if (primaryPumpActive) {
    if (primaryCurrent < config.currentThresholdMin) {
      primaryPumpState = PUMP_DRY_RUN;
      addAlert("warning", "pump_fault", "Dry Run", "Primary pump current too low - possible dry run");
    } else if (primaryCurrent > config.currentThresholdMax) {
      primaryPumpState = PUMP_FAULT;
      addAlert("warning", "pump_fault", "Overcurrent", "Primary pump current too high - possible fault");
    } else {
      primaryPumpState = PUMP_RUNNING;
    }
  } else {
    primaryPumpState = PUMP_OFF;
  }
  
  if (secondaryPumpActive) {
    if (secondaryCurrent < config.currentThresholdMin) {
      secondaryPumpState = PUMP_DRY_RUN;
      addAlert("warning", "pump_fault", "Dry Run", "Secondary pump current too low - possible dry run");
    } else if (secondaryCurrent > config.currentThresholdMax) {
      secondaryPumpState = PUMP_FAULT;
      addAlert("warning", "pump_fault", "Overcurrent", "Secondary pump current too high - possible fault");
    } else {
      secondaryPumpState = PUMP_RUNNING;
    }
  } else {
    secondaryPumpState = PUMP_OFF;
  }
}

void startPump(bool isPrimary) {
  if (emergencyStop) {
    addAlert("warning", "pump_start", "Start Failed", "Cannot start pump - emergency stop active");
    return;
  }
  
  if (isPrimary) {
    digitalWrite(PRIMARY_PUMP_RELAY, LOW);  // Active LOW
    primaryPumpActive = true;
    primaryStartTime = millis();
    addAlert("info", "pump_start", "Pump Started", "Primary pump started");
  } else {
    digitalWrite(SECONDARY_PUMP_RELAY, LOW);  // Active LOW
    secondaryPumpActive = true;
    secondaryStartTime = millis();
    addAlert("info", "pump_start", "Pump Started", "Secondary pump started");
  }
}

void stopPump(bool isPrimary) {
  if (isPrimary) {
    digitalWrite(PRIMARY_PUMP_RELAY, HIGH);  // Active LOW
    primaryPumpActive = false;
    primaryStartTime = 0;
    primaryPumpState = PUMP_OFF;
    addAlert("info", "pump_stop", "Pump Stopped", "Primary pump stopped");
  } else {
    digitalWrite(SECONDARY_PUMP_RELAY, HIGH);  // Active LOW
    secondaryPumpActive = false;
    secondaryStartTime = 0;
    secondaryPumpState = PUMP_OFF;
    addAlert("info", "pump_stop", "Pump Stopped", "Secondary pump stopped");
  }
}

void stopAllPumps() {
  stopPump(true);
  stopPump(false);
  addAlert("info", "pump_stop", "All Pumps Stopped", "All pumps stopped");
}

void updateSystemStatus() {
  // Update runtime counters and check maintenance
  checkMaintenance();
  
  // Print status to Serial for debugging
  printSystemStatus();
}

void handleReset() {
  Serial.println("Performing system reset...");
  
  // Reset system to defaults
  stopAllPumps();
  emergencyStop = false;
  currentMode = MODE_AUTO;
  
  // Clear alerts
  alertCount = 0;
  
  // Reset runtime counters
  primaryTotalRuntime = 0;
  secondaryTotalRuntime = 0;
  primaryDailyRuntime = 0;
  secondaryDailyRuntime = 0;
  
  // Reset configuration to defaults
  config.currentThresholdMin = 0.5;
  config.currentThresholdMax = 8.0;
  config.maxRuntimeMinutes = MAX_RUNTIME_MINUTES;
  config.maintenanceHours = MAINTENANCE_HOURS;
  config.autoMode = true;
  
  saveConfiguration();
  
  addAlert("info", "system_reset", "System Reset", "System has been reset to factory defaults");
}

void sendStatusResponse() {
  DynamicJsonDocument doc(1024);
  doc["mode"] = getModeString(currentMode);
  doc["emergency_stop"] = emergencyStop;
  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  doc["ip_address"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  
  sendSimpleResponse(response.c_str());
}

void sendTanksResponse() {
  DynamicJsonDocument doc(1024);
  JsonArray tanks = doc.createNestedArray("tanks");
  
  JsonObject overhead = tanks.createNestedObject();
  overhead["name"] = "overhead";
  overhead["level"] = getLevelString(readTankLevel(true));
  
  JsonObject secondary = tanks.createNestedObject();
  secondary["name"] = "secondary";
  secondary["level"] = getLevelString(readTankLevel(false));
  
  String response;
  serializeJson(doc, response);
  
  sendSimpleResponse(response.c_str());
}

void sendPumpsResponse() {
  DynamicJsonDocument doc(1024);
  JsonArray pumps = doc.createNestedArray("pumps");
  
  JsonObject primary = pumps.createNestedObject();
  primary["name"] = "primary";
  primary["state"] = getPumpStateString(primaryPumpState);
  primary["current"] = primaryCurrent;
  primary["runtime_today"] = primaryDailyRuntime;
  primary["runtime_total"] = primaryTotalRuntime;
  
  JsonObject secondary = pumps.createNestedObject();
  secondary["name"] = "secondary";
  secondary["state"] = getPumpStateString(secondaryPumpState);
  secondary["current"] = secondaryCurrent;
  secondary["runtime_today"] = secondaryDailyRuntime;
  secondary["runtime_total"] = secondaryTotalRuntime;
  
  String response;
  serializeJson(doc, response);
  
  sendSimpleResponse(response.c_str());
}

void sendAlertsResponse() {
  DynamicJsonDocument doc(1024);
  JsonArray alertsArray = doc.createNestedArray("alerts");
  
  for (int i = 0; i < alertCount; i++) {
    JsonObject alert = alertsArray.createNestedObject();
    alert["type"] = alerts[i].type;
    alert["category"] = alerts[i].category;
    alert["title"] = alerts[i].title;
    alert["message"] = alerts[i].message;
    alert["timestamp"] = alerts[i].timestamp;
  }
  
  String response;
  serializeJson(doc, response);
  
  sendSimpleResponse(response.c_str());
}

void sendConfigResponse() {
  DynamicJsonDocument doc(1024);
  doc["current_threshold_min"] = config.currentThresholdMin;
  doc["current_threshold_max"] = config.currentThresholdMax;
  doc["max_runtime_minutes"] = config.maxRuntimeMinutes;
  doc["maintenance_hours"] = config.maintenanceHours;
  doc["auto_mode"] = config.autoMode;
  
  String response;
  serializeJson(doc, response);
  
  sendSimpleResponse(response.c_str());
}

void sendSimpleResponse(const char* json, int code) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.println(" OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(json);
}
}

void startPump(bool isPrimary) {
  if (emergencyStop) {
    Serial.println("Cannot start pump - emergency stop active");
    return;
  }
  
  if (isPrimary) {
    digitalWrite(PRIMARY_PUMP_RELAY, LOW);  // Active LOW
    primaryPumpActive = true;
    primaryStartTime = millis();
    addAlert("info", "pump_start", "Pump Started", "Primary pump started");
  } else {
    digitalWrite(SECONDARY_PUMP_RELAY, LOW);  // Active LOW
    secondaryPumpActive = true;
    secondaryStartTime = millis();
    addAlert("info", "pump_start", "Pump Started", "Secondary pump started");
  }
}

void handleTestPump() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (emergencyStop) {
    server.send(400, "application/json", "{\"error\":\"Emergency stop active\"}");
    return;
  }
  
  if (server.hasArg("pump")) {
    String pumpId = server.arg("pump");
    
    // Test pump for 5 seconds
    if (pumpId == "primary") {
      Serial.println("Testing primary pump...");
      startPump(true);
      delay(5000);
      stopPump(true);
      addAlert("info", "pump_test", "Pump Test", "Primary pump test completed");
    } else if (pumpId == "secondary") {
      Serial.println("Testing secondary pump...");
      startPump(false);
      delay(5000);
      stopPump(false);
      addAlert("info", "pump_test", "Pump Test", "Secondary pump test completed");
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid pump ID\"}");
      return;
    }
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Pump ID required\"}");
  }
}

// These functions will be handled in loop() using manual HTTP request parsing

void handleReset() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  Serial.println("Performing system reset...");
  
  // Reset system to defaults
  stopAllPumps();
  emergencyStop = false;
  currentMode = MODE_AUTO;
  
  // Clear alerts
  alertCount = 0;
  
  // Reset runtime counters
  primaryTotalRuntime = 0;
  secondaryTotalRuntime = 0;
  primaryDailyRuntime = 0;
  secondaryDailyRuntime = 0;
  
  // Reset configuration to defaults
  config.currentThresholdMin = 0.5;
  config.currentThresholdMax = 8.0;
  config.maxRuntimeMinutes = MAX_RUNTIME_MINUTES;
  config.maintenanceHours = MAINTENANCE_HOURS;
  config.autoMode = true;
  
  saveConfiguration();
  
  addAlert("info", "system_reset", "System Reset", "System has been reset to factory defaults");
  
  Serial.println("System reset completed");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleEmergencyStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  Serial.println("Emergency stop activated via API");
  emergencyStop = true;
  currentMode = MODE_EMERGENCY;
  stopAllPumps();
  
  addAlert("critical", "emergency_stop", "Emergency Stop", "Emergency stop activated via remote command");
  
  server.send(200, "application/json", "{\"success\":true}");
}

// Helper functions
String getModeString(SystemMode mode) {
  switch (mode) {
    case MODE_AUTO: return "auto";
    case MODE_MANUAL: return "manual";
    case MODE_SCHEDULED: return "scheduled";
    case MODE_EMERGENCY: return "emergency";
    default: return "unknown";
  }
}

String getPumpStateString(PumpState state) {
  switch (state) {
    case PUMP_OFF: return "off";
    case PUMP_RUNNING: return "running";
    case PUMP_FAULT: return "fault";
    case PUMP_DRY_RUN: return "dry_run";
    default: return "unknown";
  }
}

String getLevelString(WaterLevel level) {
  switch (level) {
    case LEVEL_EMPTY: return "empty";
    case LEVEL_LOW: return "low";
    case LEVEL_MEDIUM: return "medium";
    case LEVEL_HIGH: return "high";
    case LEVEL_FULL: return "full";
    default: return "unknown";
  }
}

// Additional utility functions
void printSystemStatus() {
  Serial.println("=== System Status ===");
  Serial.print("Mode: ");
  Serial.println(getModeString(currentMode));
  Serial.print("Emergency Stop: ");
  Serial.println(emergencyStop ? "YES" : "NO");
  Serial.print("Primary Pump: ");
  Serial.print(getPumpStateString(primaryPumpState));
  Serial.print(" (");
  Serial.print(primaryCurrent);
  Serial.println("A)");
  Serial.print("Secondary Pump: ");
  Serial.print(getPumpStateString(secondaryPumpState));
  Serial.print(" (");
  Serial.print(secondaryCurrent);
  Serial.println("A)");
  Serial.print("Overhead Tank: ");
  Serial.println(getLevelString(readTankLevel(true)));
  Serial.print("Secondary Tank: ");
  Serial.println(getLevelString(readTankLevel(false)));
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.print("Active Alerts: ");
  Serial.println(alertCount);
  Serial.println("====================");
}

void debugSensors() {
  Serial.println("=== Sensor Debug ===");
  
  // Float switches
  Serial.println("Overhead Tank Sensors:");
  Serial.print("  Empty: "); Serial.println(!digitalRead(OVERHEAD_EMPTY_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Low: "); Serial.println(!digitalRead(OVERHEAD_LOW_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Medium: "); Serial.println(!digitalRead(OVERHEAD_MEDIUM_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  High: "); Serial.println(!digitalRead(OVERHEAD_HIGH_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Full: "); Serial.println(!digitalRead(OVERHEAD_FULL_PIN) ? "ACTIVE" : "inactive");
  
  Serial.println("Secondary Tank Sensors:");
  Serial.print("  Empty: "); Serial.println(!digitalRead(SECONDARY_EMPTY_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Low: "); Serial.println(!digitalRead(SECONDARY_LOW_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Medium: "); Serial.println(!digitalRead(SECONDARY_MEDIUM_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  High: "); Serial.println(!digitalRead(SECONDARY_HIGH_PIN) ? "ACTIVE" : "inactive");
  Serial.print("  Full: "); Serial.println(!digitalRead(SECONDARY_FULL_PIN) ? "ACTIVE" : "inactive");
  
  // Current sensors
  Serial.println("Current Sensors:");
  Serial.print("  Primary: "); Serial.print(primaryCurrent); Serial.println("A");
  Serial.print("  Secondary: "); Serial.print(secondaryCurrent); Serial.println("A");
  
  Serial.println("===================");
}

// Serial command handler for debugging
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "status") {
      printSystemStatus();
    } else if (command == "debug") {
      debugSensors();
    } else if (command == "start1") {
      startPump(true);
      Serial.println("Primary pump started manually");
    } else if (command == "start2") {
      startPump(false);
      Serial.println("Secondary pump started manually");
    } else if (command == "stop") {
      stopAllPumps();
      Serial.println("All pumps stopped manually");
    } else if (command == "auto") {
      currentMode = MODE_AUTO;
      emergencyStop = false;
      Serial.println("Auto mode enabled");
    } else if (command == "manual") {
      currentMode = MODE_MANUAL;
      Serial.println("Manual mode enabled");
    } else if (command == "emergency") {
      currentMode = MODE_EMERGENCY;
      emergencyStop = true;
      stopAllPumps();
      Serial.println("Emergency stop activated");
    } else if (command == "reset") {
      handleReset();
    } else if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("  status - Show system status");
      Serial.println("  debug - Show sensor readings");
      Serial.println("  start1 - Start primary pump");
      Serial.println("  start2 - Start secondary pump");
      Serial.println("  stop - Stop all pumps");
      Serial.println("  auto - Enable auto mode");
      Serial.println("  manual - Enable manual mode");
      Serial.println("  emergency - Emergency stop");
      Serial.println("  reset - Reset system");
      Serial.println("  help - Show this help");
    } else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

