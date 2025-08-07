# Arduino Water Level Controller - Setup Guide

## Hardware Requirements

### Components Needed
- **Arduino Uno R4 WiFi** (1x)
- **Relay Modules** (2x) - 5V, Active LOW
- **Float Switches** (10x) - Normally Open type
- **ACS712 Current Sensors** (2x) - 5A or 20A version
- **Pull-up Resistors** (10x) - 10kΩ
- **Breadboard or PCB** for connections
- **Jumper Wires** - Male-to-Male, Male-to-Female
- **Power Supply** - 12V for pumps, 5V for Arduino
- **Water Pumps** (2x) - 12V DC submersible pumps
- **Enclosure Box** - Waterproof for outdoor installation

### Tools Required
- Soldering iron and solder
- Wire strippers
- Multimeter
- Screwdrivers
- Drill (for mounting)

## Pin Configuration

### Digital Pins
```
Pin 2  - Overhead Tank Empty Float Switch
Pin 3  - Overhead Tank Low Float Switch
Pin 4  - Overhead Tank Medium Float Switch
Pin 5  - Overhead Tank High Float Switch
Pin 6  - Overhead Tank Full Float Switch
Pin 7  - Secondary Tank Empty Float Switch
Pin 8  - Secondary Tank Low Float Switch
Pin 9  - Secondary Tank Medium Float Switch
Pin 10 - Secondary Tank High Float Switch
Pin 11 - Secondary Tank Full Float Switch
Pin 12 - Primary Pump Relay Control
Pin 13 - Secondary Pump Relay Control
```

### Analog Pins
```
A0 - Primary Pump Current Sensor (ACS712)
A1 - Secondary Pump Current Sensor (ACS712)
```

## Wiring Diagram

### Float Switch Connections
```
Float Switch → Arduino Pin (with 10kΩ pull-up to 5V)
Each float switch connects between:
- One terminal to Arduino digital pin
- Other terminal to GND
- 10kΩ resistor between Arduino pin and 5V
```

### Relay Module Connections
```
Primary Pump Relay:
- VCC → 5V
- GND → GND  
- IN → Pin 12
- COM → Pump positive wire
- NO → 12V positive supply

Secondary Pump Relay:
- VCC → 5V
- GND → GND
- IN → Pin 13
- COM → Pump positive wire  
- NO → 12V positive supply
```

### Current Sensor Connections (ACS712)
```
Primary Current Sensor:
- VCC → 5V
- GND → GND
- OUT → A0
- Connect in series with primary pump power line

Secondary Current Sensor:
- VCC → 5V
- GND → GND
- OUT → A1
- Connect in series with secondary pump power line
```

## Software Setup

### 1. Arduino IDE Installation
1. Download Arduino IDE 2.0+ from [arduino.cc](https://www.arduino.cc/en/software)
2. Install the IDE following the installation wizard

### 2. Board Package Installation
1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "Arduino UNO R4 WiFi" and install

### 3. Required Libraries
Install these libraries via **Tools → Manage Libraries**:

```
1. WiFi - Built-in with Arduino UNO R4 WiFi
2. WebServer - Built-in with Arduino UNO R4 WiFi  
3. ArduinoJson - by Benoit Blanchon (version 6.x)
4. EEPROM - Built-in Arduino library
```

### 4. Code Upload
1. Open the `water_level_controller_complete.ino` file
2. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Select **Tools → Board → Arduino UNO R4 WiFi**
4. Select the correct **Port** (usually COM3, COM4, etc. on Windows)
5. Click **Upload** button

## Configuration

### 1. WiFi Setup
The system will automatically connect to your WiFi network using the credentials in the code. If connection fails, check:
- WiFi credentials are correct
- Network is 2.4GHz (not 5GHz)
- Network allows new device connections

### 2. Current Sensor Calibration
The default settings work for ACS712-5A sensors:
```cpp
#define CURRENT_SENSOR_SENSITIVITY 0.185  // 185mV/A for ACS712-5A
#define CURRENT_SENSOR_OFFSET 2.5          // 2.5V at 0A
```

For ACS712-20A sensors, change to:
```cpp
#define CURRENT_SENSOR_SENSITIVITY 0.100  // 100mV/A for ACS712-20A
```

### 3. System Parameters
Adjust these constants as needed:
```cpp
#define MAX_RUNTIME_MINUTES 60        // Maximum pump runtime
#define MAINTENANCE_HOURS 100         // Hours between maintenance alerts
#define MAX_ALERTS 20                 // Maximum stored alerts
```

## Testing and Commissioning

### 1. Initial Power-Up
1. Connect Arduino to computer via USB
2. Open Serial Monitor (115200 baud)
3. Verify system startup messages
4. Check WiFi connection status
5. Note the assigned IP address

### 2. Sensor Testing
Use serial commands to test sensors:
```
debug    - Show all sensor readings
status   - Show system status
help     - Show available commands
```

### 3. Pump Testing
Test pumps manually:
```
start1   - Start primary pump
start2   - Start secondary pump  
stop     - Stop all pumps
```

### 4. Float Switch Testing
1. Manually activate each float switch
2. Use `debug` command to verify readings
3. Check that water levels are detected correctly

### 5. Current Sensor Testing
1. Run pumps and monitor current readings
2. Verify current values are reasonable (typically 1-5A)
3. Adjust thresholds if needed via Flutter app

## API Endpoints

The Arduino creates a REST API server accessible at `http://[ARDUINO_IP]/`:

### GET Endpoints
```
/status  - Complete system status
/tanks   - Tank level information
/pumps   - Pump status and current readings
/alerts  - System alerts and notifications
/config  - Current configuration
```

### POST Endpoints
```
/pump/start?pump=primary|secondary  - Start specific pump
/pump/stop?pump=primary|secondary|all - Stop pump(s)
/pump/test?pump=primary|secondary   - Test pump for 5 seconds
/mode?mode=auto|manual|scheduled|emergency - Set system mode
/config  - Update configuration (JSON body)
/reset   - Reset system to defaults
/emergency - Emergency stop all pumps
```

## Troubleshooting

### Common Issues

#### 1. WiFi Connection Failed
- Check SSID and password
- Ensure 2.4GHz network
- Move Arduino closer to router
- Check router firewall settings

#### 2. Float Switches Not Working
- Verify wiring and pull-up resistors
- Check switch continuity with multimeter
- Ensure switches are normally open type
- Test with `debug` command

#### 3. Pumps Not Starting
- Check relay wiring (ensure Active LOW configuration)
- Verify 12V power supply to pumps
- Test relays manually
- Check pump fuses/breakers

#### 4. Current Readings Incorrect
- Verify ACS712 wiring
- Check sensor orientation (current flow direction)
- Calibrate sensor offset and sensitivity
- Ensure proper grounding

#### 5. System Resets/Crashes
- Check power supply stability
- Verify all connections are secure
- Monitor serial output for error messages
- Check for memory issues (reduce MAX_ALERTS if needed)

### Serial Commands for Debugging
```
status     - Show complete system status
debug      - Show all sensor readings
start1     - Manually start primary pump
start2     - Manually start secondary pump
stop       - Stop all pumps
auto       - Enable automatic mode
manual     - Enable manual mode
emergency  - Activate emergency stop
reset      - Reset system to defaults
help       - Show command list
```

## Safety Considerations

### Electrical Safety
- Use proper fuses and circuit breakers
- Ensure all connections are waterproof
- Use GFCI protection for pump circuits
- Keep control electronics in dry enclosure

### Water Safety
- Install overflow protection
- Use float switches as backup safety
- Test emergency stop functionality
- Regular maintenance of sensors

### System Safety
- Monitor pump runtime limits
- Check current readings regularly
- Maintain backup water supply
- Test failover functionality

## Maintenance Schedule

### Daily
- Check system status via app
- Verify pump operation
- Monitor water levels

### Weekly  
- Clean float switches
- Check electrical connections
- Test emergency stop

### Monthly
- Calibrate current sensors
- Update system configuration
- Check pump performance
- Clean intake filters

### Quarterly
- Full system test
- Replace worn components
- Update firmware if available
- Review alert logs

## Integration with Flutter App

The Arduino system is designed to work seamlessly with the Flutter mobile app:

1. **Automatic Discovery**: App can find Arduino on local network
2. **Real-time Monitoring**: Live updates of all sensors and pumps
3. **Remote Control**: Start/stop pumps, change modes
4. **Configuration**: Adjust thresholds and settings
5. **Alerts**: Receive notifications for system events
6. **Data Logging**: Historical charts and analytics

Ensure both Arduino and mobile device are on the same WiFi network for optimal performance.

## Support and Updates

For technical support or updates:
- Check serial monitor for diagnostic information
- Use built-in debugging commands
- Monitor system alerts for issues
- Keep firmware updated for new features

The system is designed for reliable, autonomous operation with minimal maintenance required.