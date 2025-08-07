# Water Level Automation System

A comprehensive Flutter application for monitoring and controlling a dual-pump water level automation system with Arduino Uno R4 WiFi integration.

## 🌊 Overview

This system provides intelligent water level monitoring and pump control for residential or commercial applications. It features dual-pump operation with automatic failover, real-time monitoring, and comprehensive safety features.

## ✨ Features

### 🏠 Dashboard
- **Real-time monitoring** of tank levels and pump status
- **Discrete level indicators** for float switch sensors (Empty, Low, Medium, High, Full)
- **Current monitoring** with efficiency calculations
- **System status** overview with connectivity indicators
- **Critical alerts** banner for immediate attention

### 🎛️ Pump Control
- **Dual pump system** (Primary & Secondary)
- **Automatic failover** when primary pump runs dry
- **Manual override** with safety interlocks
- **Pump testing** functionality
- **Emergency stop** capability
- **Multiple operation modes**: Auto, Manual, Scheduled, Emergency

### 📊 Monitoring & Analytics
- **Current draw monitoring** with visual gauges
- **Runtime tracking** (daily and total)
- **Efficiency calculations** and performance metrics
- **Historical data logging** for analysis
- **Water usage statistics** and trends
- **Maintenance scheduling** based on runtime

### 🔔 Alerts & Notifications
- **Push notifications** for remote monitoring
- **Categorized alerts**: Critical, Warning, Info
- **Alert acknowledgment** system
- **Real-time status updates**
- **Configurable notification** preferences

### ⚙️ Settings & Configuration
- **Arduino IP configuration**
- **Notification preferences**
- **System reset** capabilities
- **Connection status** monitoring

## 🏗️ System Architecture

### Hardware Components
- **Arduino Uno R4 WiFi** - Main controller
- **2x Water Pumps** - Primary and secondary pumps
- **10x Float Switches** - 5 per tank (Empty, Low, Medium, High, Full)
- **2x Current Sensors** - ACS712 or similar for pump monitoring
- **2-Channel Relay Module** - For pump control
- **RTC Module** - For scheduling functionality

### Software Architecture
```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Flutter App   │◄──►│  Arduino R4 WiFi │◄──►│  Hardware Layer │
│                 │    │                  │    │                 │
│ • Dashboard     │    │ • REST API       │    │ • Pumps         │
│ • Controls      │    │ • Sensor Reading │    │ • Sensors       │
│ • Analytics     │    │ • Pump Control   │    │ • Relays        │
│ • Alerts        │    │ • Safety Logic   │    │ • Current Mon.  │
│ • Settings      │    │ • Scheduling     │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 🚀 Getting Started

### Prerequisites
- Flutter SDK (3.8.1 or higher)
- Arduino IDE
- Arduino Uno R4 WiFi
- Required hardware components

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd water_level_automation
   ```

2. **Install Flutter dependencies**
   ```bash
   flutter pub get
   ```

3. **Configure Arduino IP**
   - Update the IP address in the app settings
   - Default: `192.168.1.197`

4. **Build and run**
   ```bash
   # For web
   flutter build web
   
   # For mobile (requires Developer Mode on Windows)
   flutter run
   ```

## 📱 App Structure

### Core Components

#### Models
- [`TankLevel`](lib/models/tank_level.dart) - Water level data with discrete sensors
- [`PumpStatus`](lib/models/pump_status.dart) - Pump state and performance metrics
- [`SystemStatus`](lib/models/system_status.dart) - Overall system health
- [`Alert`](lib/models/alert.dart) - Notification and alert management
- [`Schedule`](lib/models/schedule.dart) - Time-based automation

#### Services
- [`ArduinoService`](lib/services/arduino_service.dart) - REST API communication
- [`NotificationService`](lib/services/notification_service.dart) - Local notifications
- [`DataLogger`](lib/services/data_logger.dart) - Historical data and analytics

#### Screens
- [`DashboardScreen`](lib/screens/dashboard_screen.dart) - Main monitoring interface
- [`ControlScreen`](lib/screens/control_screen.dart) - Manual pump operations
- [`ScheduleScreen`](lib/screens/schedule_screen.dart) - Automation scheduling
- [`AnalyticsScreen`](lib/screens/analytics_screen.dart) - Usage statistics
- [`AlertsScreen`](lib/screens/alerts_screen.dart) - Alert management
- [`SettingsScreen`](lib/screens/settings_screen.dart) - System configuration

#### Custom Widgets
- [`TankLevelWidget`](lib/widgets/tank_level_widget.dart) - Discrete level visualization
- [`PumpStatusWidget`](lib/widgets/pump_status_widget.dart) - Current monitoring gauges
- [`SystemStatusWidget`](lib/widgets/system_status_widget.dart) - System health overview
- [`QuickActionsWidget`](lib/widgets/quick_actions_widget.dart) - Emergency controls

## 🔌 Arduino REST API

### Endpoints

#### Status & Monitoring
- `GET /status` - Complete system status
- `GET /tanks` - Tank level readings
- `GET /pumps` - Pump status and current readings
- `GET /alerts` - Active system alerts

#### Pump Control
- `POST /pump/start/{pump_id}` - Start specific pump
- `POST /pump/stop` - Stop all pumps
- `POST /pump/switch` - Switch between pumps
- `POST /pump/test/{pump_id}` - Test pump operation

#### System Modes
- `POST /mode/auto` - Enable automatic mode
- `POST /mode/manual` - Enable manual override
- `POST /mode/scheduled` - Enable scheduled mode
- `POST /mode/emergency` - Emergency stop all operations

#### Configuration
- `POST /config` - Update system configuration
- `GET /config` - Get current configuration
- `POST /reset` - Reset system to defaults

### Data Formats

#### Tank Level Response
```json
{
  "tank_id": "overhead",
  "level": "medium",
  "level_numeric": 2,
  "timestamp": "2024-01-01T12:00:00Z",
  "sensors": {
    "empty": false,
    "low": false,
    "medium": true,
    "high": false,
    "full": false
  }
}
```

#### Pump Status Response
```json
{
  "pump_id": "primary",
  "status": "running",
  "current_draw": 4.2,
  "runtime_today": 3600,
  "total_runtime": 86400,
  "last_started": "2024-01-01T10:00:00Z",
  "maintenance_due": false
}
```

## 🛡️ Safety Features

### Hardware Safety
- **Automatic pump stop** when overhead tank is full
- **Dry run protection** via current monitoring
- **Maximum runtime limits** to prevent overheating
- **Emergency stop** functionality

### Software Safety
- **Watchdog timer** for Arduino communication
- **Failsafe mode** if communication is lost
- **User confirmation** for critical operations
- **Backup schedule** storage on Arduino

### Alert System
- **Critical alerts**: Tank empty, pump failure, communication loss
- **Warning alerts**: Low levels, high consumption, maintenance due
- **Info alerts**: Scheduled operations, mode changes

## 📊 Analytics & Data

### Tracked Metrics
- **Water usage** (daily, weekly, monthly)
- **Pump runtime** and efficiency
- **Power consumption** analysis
- **System uptime** and reliability
- **Alert frequency** and patterns

### Data Storage
- **Local storage** using SharedPreferences
- **Historical data** with configurable retention
- **Export capabilities** for external analysis
- **Automatic cleanup** of old data

## 🔧 Configuration

### System Settings
- **Arduino IP address** configuration
- **Notification preferences** (Critical, Warning, Info)
- **Auto-refresh intervals**
- **Data retention periods**

### Pump Settings
- **Runtime limits** (daily maximum)
- **Maintenance intervals** (hours-based)
- **Current thresholds** for fault detection
- **Efficiency targets**

## 🚨 Troubleshooting

### Common Issues

1. **Connection Failed**
   - Check Arduino IP address
   - Verify WiFi connectivity
   - Ensure Arduino is powered on

2. **Pump Not Starting**
   - Check manual override mode
   - Verify tank levels meet requirements
   - Check for active alerts

3. **Inaccurate Readings**
   - Calibrate float switches
   - Check sensor connections
   - Verify current sensor placement

### Debug Mode
- Enable debug logging in settings
- Check connection status indicators
- Use test pump functionality

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📞 Support

For support and questions:
- Create an issue in the repository
- Check the troubleshooting section
- Review the system architecture documentation

---

**Built with Flutter & Arduino for intelligent water management** 🌊
