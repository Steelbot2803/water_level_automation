// Remaining Arduino functions for complete_controller.ino

void handleTestPump(String pumpId) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (emergencyStop) {
    server.send(400, "application/json", "{\"error\":\"Emergency stop active\"}");
    return;
  }
  
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
}

void handleSetMode(String mode) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  Serial.println("Setting mode to: " + mode);
  
  if (mode == "auto") {
    currentMode = MODE_AUTO;
    emergencyStop = false;
    addAlert("info", "mode_change", "Mode Changed", "System set to automatic mode");
  } else if (mode == "manual") {
    currentMode = MODE_MANUAL;
    emergencyStop = false;
    addAlert("info", "mode_change", "Mode Changed", "System set to manual mode");
  } else if (mode == "scheduled") {
    currentMode = MODE_SCHEDULED;
    emergencyStop = false;
    addAlert("info", "mode_change", "Mode Changed", "System set to scheduled mode");
  } else if (mode == "emergency") {
    currentMode = MODE_EMERGENCY;
    emergencyStop = true;
    stopAllPumps();
    addAlert("critical", "emergency_stop", "Emergency Stop", "Emergency stop activated - all pumps stopped");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid mode\"}");
    return;
  }
  
  server.send(200, "application/json", "{\"success\":true}");
}

void handleGetConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  DynamicJsonDocument doc(1024);
  doc["wifi_ssid"] = config.wifiSSID;
  doc["current_threshold_min"] = config.currentThresholdMin;
  doc["current_threshold_max"] = config.currentThresholdMax;
  doc["max_runtime_minutes"] = config.maxRuntimeMinutes;
  doc["maintenance_hours"] = config.maintenanceHours;
  doc["auto_mode"] = config.autoMode;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    bool configChanged = false;
    
    if (doc.containsKey("current_threshold_min")) {
      config.currentThresholdMin = doc["current_threshold_min"];
      configChanged = true;
    }
    if (doc.containsKey("current_threshold_max")) {
      config.currentThresholdMax = doc["current_threshold_max"];
      configChanged = true;
    }
    if (doc.containsKey("max_runtime_minutes")) {
      config.maxRuntimeMinutes = doc["max_runtime_minutes"];
      configChanged = true;
    }
    if (doc.containsKey("maintenance_hours")) {
      config.maintenanceHours = doc["maintenance_hours"];
      configChanged = true;
    }
    if (doc.containsKey("auto_mode")) {
      config.autoMode = doc["auto_mode"];
      configChanged = true;
    }
    
    if (configChanged) {
      saveConfiguration();
      addAlert("info", "config_update", "Configuration Updated", "System configuration has been updated");
      Serial.println("Configuration updated");
    }
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No configuration data provided\"}");
  }
}

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