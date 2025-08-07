import 'dart:async';
import 'package:flutter/material.dart';
import '../models/tank_level.dart';
import '../models/pump_status.dart';
import '../models/system_status.dart';
import '../models/alert.dart';
import '../models/schedule.dart';
import '../services/arduino_service.dart';
import '../services/notification_service.dart';
import '../services/data_logger.dart';

class WaterSystemProvider extends ChangeNotifier {
  final ArduinoService _arduinoService = ArduinoService();
  final NotificationService _notificationService = NotificationService();
  final DataLogger _dataLogger = DataLogger();

  // State variables
  List<TankLevel> _tankLevels = [];
  List<PumpStatus> _pumpStatuses = [];
  SystemStatus? _systemStatus;
  List<Alert> _alerts = [];
  List<Schedule> _schedules = [];
  
  bool _isLoading = false;
  bool _isConnected = false;
  String? _errorMessage;
  DateTime? _lastUpdate;

  // Getters
  List<TankLevel> get tankLevels => _tankLevels;
  List<PumpStatus> get pumpStatuses => _pumpStatuses;
  SystemStatus? get systemStatus => _systemStatus;
  List<Alert> get alerts => _alerts;
  List<Schedule> get schedules => _schedules;
  bool get isLoading => _isLoading;
  bool get isConnected => _isConnected;
  String? get errorMessage => _errorMessage;
  DateTime? get lastUpdate => _lastUpdate;

  // Convenience getters
  TankLevel? get overheadTank => _tankLevels.firstWhere(
    (tank) => tank.tankId == 'overhead',
    orElse: () => TankLevel(
      tankId: 'overhead',
      level: WaterLevel.empty,
      levelNumeric: 0,
      timestamp: DateTime.now(),
      sensors: {},
    ),
  );

  TankLevel? get secondaryTank => _tankLevels.firstWhere(
    (tank) => tank.tankId == 'secondary',
    orElse: () => TankLevel(
      tankId: 'secondary',
      level: WaterLevel.empty,
      levelNumeric: 0,
      timestamp: DateTime.now(),
      sensors: {},
    ),
  );

  PumpStatus? get primaryPump => _pumpStatuses.firstWhere(
    (pump) => pump.pumpId == PumpType.primary,
    orElse: () => PumpStatus(
      pumpId: PumpType.primary,
      status: PumpState.off,
      currentDraw: 0.0,
      runtimeToday: 0,
      totalRuntime: 0,
      maintenanceDue: false,
    ),
  );

  PumpStatus? get secondaryPump => _pumpStatuses.firstWhere(
    (pump) => pump.pumpId == PumpType.secondary,
    orElse: () => PumpStatus(
      pumpId: PumpType.secondary,
      status: PumpState.off,
      currentDraw: 0.0,
      runtimeToday: 0,
      totalRuntime: 0,
      maintenanceDue: false,
    ),
  );

  List<Alert> get criticalAlerts => _alerts.where((alert) => 
    alert.type == AlertType.critical && !alert.acknowledged).toList();

  List<Alert> get warningAlerts => _alerts.where((alert) => 
    alert.type == AlertType.warning && !alert.acknowledged).toList();

  bool get hasActivePump => _pumpStatuses.any((pump) => pump.isRunning);
  bool get hasCriticalAlerts => criticalAlerts.isNotEmpty;
  bool get hasWarningAlerts => warningAlerts.isNotEmpty;

  // Initialize the provider
  Future<void> initialize() async {
    await _notificationService.initialize();
    _startListening();
    await refreshData();
  }

  // Set Arduino IP address
  void setArduinoIP(String ip) {
    _arduinoService.setBaseUrl(ip);
    notifyListeners();
  }

  // Refresh all data
  Future<void> refreshData() async {
    if (_isLoading) return;

    _setLoading(true);
    _clearError();

    try {
      // Test connection first
      _isConnected = await _arduinoService.testConnection();
      
      if (!_isConnected) {
        throw Exception('Cannot connect to Arduino');
      }

      // Fetch all data concurrently
      final futures = await Future.wait([
        _arduinoService.getTankLevels(),
        _arduinoService.getPumpStatus(),
        _arduinoService.getAlerts(),
        _arduinoService.getSchedules(),
      ]);

      _tankLevels = futures[0] as List<TankLevel>;
      _pumpStatuses = futures[1] as List<PumpStatus>;
      _alerts = futures[2] as List<Alert>;
      _schedules = futures[3] as List<Schedule>;

      // Get system status
      final statusData = await _arduinoService.getSystemStatus();
      _systemStatus = SystemStatus.fromJson(statusData);

      // Log data for analytics
      await _dataLogger.logTankData(_tankLevels);
      await _dataLogger.logPumpData(_pumpStatuses);
      if (_systemStatus != null) {
        await _dataLogger.logSystemData(_systemStatus!);
      }

      // Process alerts
      await _processAlerts();

      _lastUpdate = DateTime.now();
      _isConnected = true;

    } catch (e) {
      _setError(e.toString());
      _isConnected = false;
    } finally {
      _setLoading(false);
    }
  }

  // Pump control methods
  Future<bool> startPump(PumpType pumpType) async {
    try {
      final pumpId = pumpType == PumpType.primary ? 'primary' : 'secondary';
      final success = await _arduinoService.startPump(pumpId);
      
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Pump Started',
          message: '${pumpType.name.toUpperCase()} pump has been started',
          type: AlertType.info,
        );
        await refreshData();
      }
      
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> stopPump() async {
    try {
      final success = await _arduinoService.stopPump();
      
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Pump Stopped',
          message: 'All pumps have been stopped',
          type: AlertType.info,
        );
        await refreshData();
      }
      
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> switchPump() async {
    try {
      final success = await _arduinoService.switchPump();
      
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Pump Switched',
          message: 'Switched to backup pump',
          type: AlertType.info,
        );
        await refreshData();
      }
      
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> testPump(PumpType pumpType) async {
    try {
      final pumpId = pumpType == PumpType.primary ? 'primary' : 'secondary';
      final success = await _arduinoService.testPump(pumpId);
      
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Pump Test',
          message: '${pumpType.name.toUpperCase()} pump test initiated',
          type: AlertType.info,
        );
        await refreshData();
      }
      
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  // System mode control
  Future<bool> setAutoMode() async {
    try {
      final success = await _arduinoService.setAutoMode();
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Mode Changed',
          message: 'System set to automatic mode',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> setManualMode() async {
    try {
      final success = await _arduinoService.setManualMode();
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Mode Changed',
          message: 'System set to manual mode',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> setScheduledMode() async {
    try {
      final success = await _arduinoService.setScheduledMode();
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Mode Changed',
          message: 'System set to scheduled mode',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> emergencyStop() async {
    try {
      final success = await _arduinoService.emergencyStop();
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Emergency Stop',
          message: 'Emergency stop activated - all pumps stopped',
          type: AlertType.critical,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  // Schedule management
  Future<bool> addSchedule(Schedule schedule) async {
    try {
      final success = await _arduinoService.setSchedule(schedule);
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Schedule Added',
          message: 'New schedule "${schedule.name}" has been added',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> deleteSchedule(String scheduleId) async {
    try {
      final success = await _arduinoService.deleteSchedule(scheduleId);
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Schedule Deleted',
          message: 'Schedule has been removed',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  Future<bool> toggleSchedule(String scheduleId, bool enabled) async {
    try {
      final success = await _arduinoService.enableSchedule(scheduleId, enabled);
      if (success) {
        await _notificationService.showSystemNotification(
          title: 'Schedule ${enabled ? 'Enabled' : 'Disabled'}',
          message: 'Schedule has been ${enabled ? 'enabled' : 'disabled'}',
          type: AlertType.info,
        );
        await refreshData();
      }
      return success;
    } catch (e) {
      _setError(e.toString());
      return false;
    }
  }

  // Alert management
  Future<void> acknowledgeAlert(String alertId) async {
    final alertIndex = _alerts.indexWhere((alert) => alert.id == alertId);
    if (alertIndex != -1) {
      _alerts[alertIndex] = _alerts[alertIndex].copyWith(acknowledged: true);
      notifyListeners();
    }
  }

  Future<void> clearAllAlerts() async {
    for (int i = 0; i < _alerts.length; i++) {
      _alerts[i] = _alerts[i].copyWith(acknowledged: true);
    }
    notifyListeners();
  }

  // Data and analytics
  Future<UsageStatistics> getUsageStatistics({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    return await _dataLogger.getUsageStatistics(
      startDate: startDate,
      endDate: endDate,
    );
  }

  Future<List<DataPoint>> getDailyUsage({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    return await _dataLogger.getDailyUsage(
      startDate: startDate,
      endDate: endDate,
    );
  }

  // Private methods
  void _startListening() {
    // Listen to Arduino status updates
    _arduinoService.statusStream.listen((data) {
      // Handle real-time updates
    });

    // Listen to notification alerts
    _notificationService.alertStream.listen((alert) {
      _alerts.insert(0, alert);
      notifyListeners();
    });

    // Start auto-refresh
    _arduinoService.startAutoRefresh();
  }

  Future<void> _processAlerts() async {
    // Check for critical conditions and send notifications
    for (final tank in _tankLevels) {
      if (tank.isCritical) {
        await _notificationService.showTankAlert(
          tankName: tank.tankId == 'overhead' ? 'Overhead Tank' : 'Secondary Tank',
          level: tank.level.name,
          type: AlertType.critical,
        );
      } else if (tank.isWarning) {
        await _notificationService.showTankAlert(
          tankName: tank.tankId == 'overhead' ? 'Overhead Tank' : 'Secondary Tank',
          level: tank.level.name,
          type: AlertType.warning,
        );
      }
    }

    // Check pump conditions
    for (final pump in _pumpStatuses) {
      if (pump.hasFault) {
        await _notificationService.showPumpAlert(
          pumpName: pump.pumpName,
          status: pump.status.name,
          type: AlertType.critical,
          faultCode: pump.faultCode,
        );
      }

      if (pump.maintenanceDue) {
        await _notificationService.showMaintenanceAlert(
          pumpName: pump.pumpName,
          totalHours: pump.totalRuntime ~/ 3600,
        );
      }
    }
  }

  void _setLoading(bool loading) {
    _isLoading = loading;
    notifyListeners();
  }

  void _setError(String error) {
    _errorMessage = error;
    notifyListeners();
  }

  void _clearError() {
    _errorMessage = null;
    notifyListeners();
  }

  @override
  void dispose() {
    _arduinoService.dispose();
    _notificationService.dispose();
    super.dispose();
  }
}