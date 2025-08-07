// Continuation of water_level_controller.ino

void handleTestPump(String pumpId) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (emergencyStop) {
    server.send(400, "application/json", "{\"error\":\"Emergency stop active\"}");
    return;
  }
  
  // Test pump for 5 seconds
  if (pumpId == "primary") {
    startPump(true);
    delay(5000);
    stopPump(true);
    addAlert("info", "pump_test", "Pump Test", "Primary pump test completed");
  } else if (pumpId == "secondary") {
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
  
  if (mode == "auto") {
    currentMode = MODE_AUTO;
    emergencyStop = false;
  } else if (mode == "manual") {
    currentMode = MODE_MANUAL;
    emergencyStop = false;
  } else if (mode == "scheduled") {
    currentMode = MODE_SCHEDULED;
    emergencyStop = false;
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
    
    if (doc.containsKey("current_threshold_min")) {
      config.currentThresholdMin = doc["current_threshold_min"];
    }
    if (doc.containsKey("current_threshold_max")) {
      config.currentThresholdMax = doc["current_threshold_max"];
    }
    if (doc.containsKey("max_runtime_minutes")) {
      config.maxRuntimeMinutes = doc["max_runtime_minutes"];
    }
    if (doc.containsKey("maintenance_hours")) {
      config.maintenanceHours = doc["maintenance_hours"];
    }
    if (doc.containsKey("auto_mode")) {
      config.autoMode = doc["auto_mode"];
    }
    
    saveConfiguration();
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No configuration data provided\"}");
  }
}

void handleReset() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  // Reset system to defaults
  stopAllPumps();
  emergencyStop = false;
  currentMode = MODE_AUTO;
  
  // Clear alerts
  activeAlerts.clear();
  
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