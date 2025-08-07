# Flutter Water Level Monitoring App - Setup Guide

## Overview

The Flutter mobile application provides a comprehensive interface for monitoring and controlling the Arduino-based water level automation system. The app offers real-time monitoring, remote control, configuration management, and data analytics.

## Features

### Core Functionality
- **Real-time Monitoring**: Live updates of tank levels and pump status
- **Remote Control**: Start/stop pumps, change system modes
- **Alert System**: Push notifications for critical events
- **Data Logging**: Historical charts and usage analytics
- **Configuration**: Adjust system parameters and thresholds
- **Scheduling**: Time-based pump control
- **Maintenance**: Runtime tracking and maintenance alerts

### User Interface
- **Dashboard**: Overview of entire system status
- **Tank Monitoring**: Discrete level indicators with float switch status
- **Pump Control**: Current monitoring with gauge displays
- **Settings**: System configuration and preferences
- **Analytics**: Historical data and usage patterns
- **Alerts**: Notification management and history

## Prerequisites

### Development Environment
- **Flutter SDK**: Version 3.0 or higher
- **Dart SDK**: Version 2.17 or higher
- **Android Studio** or **VS Code** with Flutter extensions
- **Git** for version control

### Target Platforms
- **Android**: API level 21+ (Android 5.0+)
- **iOS**: iOS 11.0+
- **Web**: Modern browsers with CORS support

## Installation

### 1. Flutter SDK Setup

#### Windows
```bash
# Download Flutter SDK from https://flutter.dev/docs/get-started/install
# Extract to C:\flutter
# Add C:\flutter\bin to PATH environment variable

# Verify installation
flutter doctor
```

#### macOS
```bash
# Using Homebrew
brew install flutter

# Or download from https://flutter.dev/docs/get-started/install
# Extract and add to PATH

# Verify installation
flutter doctor
```

#### Linux
```bash
# Download Flutter SDK
wget https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.x.x-stable.tar.xz

# Extract and add to PATH
tar xf flutter_linux_3.x.x-stable.tar.xz
export PATH="$PATH:`pwd`/flutter/bin"

# Verify installation
flutter doctor
```

### 2. Project Setup

#### Clone and Setup
```bash
# Navigate to project directory
cd water_level_automation

# Get Flutter dependencies
flutter pub get

# Verify project structure
flutter analyze
```

#### Dependencies
The app uses these key dependencies (already configured in `pubspec.yaml`):

```yaml
dependencies:
  flutter:
    sdk: flutter
  http: ^1.1.0                    # HTTP requests to Arduino
  provider: ^6.1.1                # State management
  shared_preferences: ^2.2.2      # Local storage
  fl_chart: ^0.68.0               # Charts and graphs
  flutter_local_notifications: ^17.2.1  # Push notifications
  intl: ^0.19.0                   # Date/time formatting
  connectivity_plus: ^5.0.2       # Network connectivity
  permission_handler: ^11.3.1     # Device permissions
```

### 3. Configuration

#### Network Configuration
Update the Arduino IP address in [`lib/services/arduino_service.dart`](lib/services/arduino_service.dart):

```dart
class ArduinoService {
  static const String baseUrl = 'http://192.168.1.100'; // Update with your Arduino IP
  
  // Alternative: Use mDNS discovery
  static const String baseUrl = 'http://water-controller.local';
}
```

#### Notification Setup

##### Android
Add to `android/app/src/main/AndroidManifest.xml`:
```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.RECEIVE_BOOT_COMPLETED" />
<uses-permission android:name="android.permission.VIBRATE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
```

##### iOS
Add to `ios/Runner/Info.plist`:
```xml
<key>NSLocalNetworkUsageDescription</key>
<string>This app needs access to local network to communicate with water level controller</string>
```

## Building the App

### Development Build
```bash
# Run on connected device/emulator
flutter run

# Run with hot reload for development
flutter run --hot
```

### Production Builds

#### Android APK
```bash
# Build release APK
flutter build apk --release

# Build App Bundle (recommended for Play Store)
flutter build appbundle --release
```

#### iOS
```bash
# Build for iOS (requires macOS and Xcode)
flutter build ios --release
```

#### Web
```bash
# Build for web deployment
flutter build web --release
```

## App Architecture

### State Management
The app uses the **Provider** pattern for state management:

```dart
// Main provider: WaterSystemProvider
class WaterSystemProvider extends ChangeNotifier {
  // System state
  SystemStatus? _systemStatus;
  List<Alert> _alerts = [];
  bool _isConnected = false;
  
  // Methods for updating state
  Future<void> fetchSystemStatus() async { ... }
  Future<void> startPump(String pumpId) async { ... }
  Future<void> setSystemMode(String mode) async { ... }
}
```

### Service Layer
- **ArduinoService**: HTTP communication with Arduino
- **NotificationService**: Local push notifications
- **DataLogger**: Historical data storage and retrieval

### UI Components
- **Screens**: Main app pages (Dashboard, Settings, etc.)
- **Widgets**: Reusable UI components (Tank display, Pump controls)
- **Models**: Data structures for system entities

## Key Features Implementation

### 1. Real-time Monitoring
```dart
// Auto-refresh system status every 5 seconds
Timer.periodic(Duration(seconds: 5), (timer) {
  if (mounted) {
    context.read<WaterSystemProvider>().fetchSystemStatus();
  }
});
```

### 2. Tank Level Display
The app shows discrete water levels using float switch data:
```dart
// Tank level widget with 5 discrete levels
Widget buildTankLevel(TankLevel tankLevel) {
  return Column(
    children: [
      _buildLevelIndicator('Full', tankLevel.full),
      _buildLevelIndicator('High', tankLevel.high),
      _buildLevelIndicator('Medium', tankLevel.medium),
      _buildLevelIndicator('Low', tankLevel.low),
      _buildLevelIndicator('Empty', tankLevel.empty),
    ],
  );
}
```

### 3. Current Monitoring
Pump current is displayed using gauge widgets:
```dart
// Current gauge with color-coded zones
Widget buildCurrentGauge(double current) {
  return SfRadialGauge(
    axes: [
      RadialAxis(
        minimum: 0,
        maximum: 10,
        ranges: [
          GaugeRange(startValue: 0, endValue: 2, color: Colors.green),
          GaugeRange(startValue: 2, endValue: 8, color: Colors.blue),
          GaugeRange(startValue: 8, endValue: 10, color: Colors.red),
        ],
        pointers: [NeedlePointer(value: current)],
      ),
    ],
  );
}
```

### 4. Push Notifications
```dart
// Initialize notifications
await FlutterLocalNotificationsPlugin().initialize(
  InitializationSettings(
    android: AndroidInitializationSettings('@mipmap/ic_launcher'),
    iOS: DarwinInitializationSettings(),
  ),
);

// Show notification for alerts
void showAlert(Alert alert) {
  FlutterLocalNotificationsPlugin().show(
    alert.hashCode,
    alert.title,
    alert.message,
    NotificationDetails(
      android: AndroidNotificationDetails(
        'water_alerts',
        'Water System Alerts',
        importance: alert.type == 'critical' 
          ? Importance.high 
          : Importance.defaultImportance,
      ),
    ),
  );
}
```

## Screen Descriptions

### 1. Dashboard Screen
**File**: [`lib/screens/dashboard_screen.dart`](lib/screens/dashboard_screen.dart)

**Features**:
- System overview with key metrics
- Tank level indicators
- Pump status and controls
- Recent alerts
- Quick action buttons

**Widgets Used**:
- [`SystemStatusWidget`](lib/widgets/system_status_widget.dart)
- [`TankLevelWidget`](lib/widgets/tank_level_widget.dart)
- [`PumpStatusWidget`](lib/widgets/pump_status_widget.dart)

### 2. Tank Monitoring
**Features**:
- Detailed tank level display
- Individual float switch status
- Historical level charts
- Water usage analytics

### 3. Pump Control
**Features**:
- Manual pump control
- Current monitoring gauges
- Runtime statistics
- Maintenance alerts

### 4. Settings Screen
**Features**:
- System configuration
- Current thresholds
- Runtime limits
- WiFi settings
- Notification preferences

### 5. Analytics Screen
**Features**:
- Historical charts
- Usage patterns
- Efficiency metrics
- Export data functionality

## API Integration

### HTTP Communication
The app communicates with Arduino via REST API:

```dart
class ArduinoService {
  static const String baseUrl = 'http://192.168.1.100';
  
  // Get system status
  Future<SystemStatus> getSystemStatus() async {
    final response = await http.get(Uri.parse('$baseUrl/status'));
    if (response.statusCode == 200) {
      return SystemStatus.fromJson(json.decode(response.body));
    }
    throw Exception('Failed to get system status');
  }
  
  // Start pump
  Future<void> startPump(String pumpId) async {
    final response = await http.post(
      Uri.parse('$baseUrl/pump/start?pump=$pumpId'),
    );
    if (response.statusCode != 200) {
      throw Exception('Failed to start pump');
    }
  }
  
  // Set system mode
  Future<void> setMode(String mode) async {
    final response = await http.post(
      Uri.parse('$baseUrl/mode?mode=$mode'),
    );
    if (response.statusCode != 200) {
      throw Exception('Failed to set mode');
    }
  }
}
```

### Error Handling
```dart
// Robust error handling with retry logic
Future<T> _makeRequest<T>(Future<T> Function() request) async {
  int retries = 3;
  while (retries > 0) {
    try {
      return await request();
    } catch (e) {
      retries--;
      if (retries == 0) rethrow;
      await Future.delayed(Duration(seconds: 2));
    }
  }
  throw Exception('Max retries exceeded');
}
```

## Data Models

### System Status
```dart
class SystemStatus {
  final String mode;
  final bool emergencyStop;
  final bool wifiConnected;
  final TankLevel overheadTank;
  final TankLevel secondaryTank;
  final PumpStatus primaryPump;
  final PumpStatus secondaryPump;
  final int alertCount;
  
  SystemStatus({...});
  
  factory SystemStatus.fromJson(Map<String, dynamic> json) => SystemStatus(...);
}
```

### Tank Level
```dart
class TankLevel {
  final String level;
  final int levelNumeric;
  final Map<String, bool> sensors;
  
  TankLevel({...});
  
  // Discrete level properties
  bool get empty => sensors['empty'] ?? false;
  bool get low => sensors['low'] ?? false;
  bool get medium => sensors['medium'] ?? false;
  bool get high => sensors['high'] ?? false;
  bool get full => sensors['full'] ?? false;
}
```

### Pump Status
```dart
class PumpStatus {
  final String state;
  final bool active;
  final double current;
  final int dailyRuntime;
  final int totalRuntime;
  
  PumpStatus({...});
  
  // Current status helpers
  bool get isRunning => state == 'running';
  bool get hasFault => state == 'fault';
  bool get isDryRun => state == 'dry_run';
}
```

## Testing

### Unit Tests
```bash
# Run unit tests
flutter test

# Run with coverage
flutter test --coverage
```

### Integration Tests
```bash
# Run integration tests
flutter test integration_test/
```

### Widget Tests
```bash
# Test specific widgets
flutter test test/widgets/
```

## Deployment

### Android Play Store
1. Build signed APK or App Bundle
2. Create Play Store listing
3. Upload build and submit for review

### iOS App Store
1. Build for iOS with proper certificates
2. Create App Store Connect listing
3. Upload via Xcode or Application Loader

### Web Deployment
1. Build web version: `flutter build web`
2. Deploy to hosting service (Firebase, Netlify, etc.)
3. Configure CORS for Arduino communication

## Troubleshooting

### Common Issues

#### 1. Cannot Connect to Arduino
- Verify Arduino IP address in `arduino_service.dart`
- Check WiFi network connectivity
- Ensure Arduino and phone are on same network
- Test Arduino API endpoints in browser

#### 2. Notifications Not Working
- Check notification permissions
- Verify notification service initialization
- Test with simple notification first
- Check device notification settings

#### 3. Charts Not Displaying
- Verify data format from Arduino
- Check chart widget configuration
- Ensure sufficient data points
- Test with mock data first

#### 4. App Crashes on Startup
- Check Flutter doctor output
- Verify all dependencies are compatible
- Clear app data and reinstall
- Check device logs for specific errors

### Debug Tools
```bash
# Enable debug logging
flutter run --verbose

# Inspect widget tree
flutter inspector

# Performance profiling
flutter run --profile
```

## Performance Optimization

### Network Optimization
- Implement request caching
- Use connection pooling
- Minimize API call frequency
- Handle offline scenarios

### UI Optimization
- Use `const` constructors where possible
- Implement lazy loading for lists
- Optimize image assets
- Use `RepaintBoundary` for complex widgets

### Memory Management
- Dispose controllers and streams
- Use weak references where appropriate
- Monitor memory usage during development
- Implement proper lifecycle management

## Security Considerations

### Network Security
- Use HTTPS where possible
- Implement request authentication
- Validate all API responses
- Handle sensitive data securely

### App Security
- Obfuscate release builds
- Implement certificate pinning
- Secure local data storage
- Regular security updates

## Maintenance

### Regular Updates
- Keep Flutter SDK updated
- Update dependencies regularly
- Monitor for security vulnerabilities
- Test on latest OS versions

### Monitoring
- Implement crash reporting
- Monitor API response times
- Track user engagement metrics
- Log critical system events

The Flutter app provides a comprehensive, user-friendly interface for the water level automation system, enabling remote monitoring and control with real-time updates and intelligent alerts.