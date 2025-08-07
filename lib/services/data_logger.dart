import 'dart:convert';
import 'dart:async';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/tank_level.dart';
import '../models/pump_status.dart';
import '../models/system_status.dart';
import '../models/alert.dart';

class DataPoint {
  final DateTime timestamp;
  final Map<String, dynamic> data;

  DataPoint({
    required this.timestamp,
    required this.data,
  });

  factory DataPoint.fromJson(Map<String, dynamic> json) {
    return DataPoint(
      timestamp: DateTime.parse(json['timestamp']),
      data: json['data'],
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'timestamp': timestamp.toIso8601String(),
      'data': data,
    };
  }
}

class UsageStatistics {
  final double totalWaterPumped;
  final int totalPumpRuntime;
  final double averageDailyUsage;
  final double peakHourUsage;
  final Map<String, int> pumpUsageBreakdown;
  final List<DataPoint> hourlyUsage;

  UsageStatistics({
    required this.totalWaterPumped,
    required this.totalPumpRuntime,
    required this.averageDailyUsage,
    required this.peakHourUsage,
    required this.pumpUsageBreakdown,
    required this.hourlyUsage,
  });
}

class DataLogger {
  static const String _tankDataKey = 'tank_data_log';
  static const String _pumpDataKey = 'pump_data_log';
  static const String _systemDataKey = 'system_data_log';
  static const String _alertDataKey = 'alert_data_log';
  static const String _usageDataKey = 'usage_data_log';
  
  static const int _maxDataPoints = 1000; // Keep last 1000 data points
  static const int _maxAlerts = 500; // Keep last 500 alerts

  // Log tank level data
  Future<void> logTankData(List<TankLevel> tankLevels) async {
    final prefs = await SharedPreferences.getInstance();
    final existingData = await _getDataPoints(_tankDataKey);
    
    final dataPoint = DataPoint(
      timestamp: DateTime.now(),
      data: {
        'tanks': tankLevels.map((tank) => tank.toJson()).toList(),
      },
    );
    
    existingData.add(dataPoint);
    
    // Keep only the latest data points
    if (existingData.length > _maxDataPoints) {
      existingData.removeRange(0, existingData.length - _maxDataPoints);
    }
    
    await _saveDataPoints(_tankDataKey, existingData);
  }

  // Log pump status data
  Future<void> logPumpData(List<PumpStatus> pumpStatuses) async {
    final prefs = await SharedPreferences.getInstance();
    final existingData = await _getDataPoints(_pumpDataKey);
    
    final dataPoint = DataPoint(
      timestamp: DateTime.now(),
      data: {
        'pumps': pumpStatuses.map((pump) => pump.toJson()).toList(),
      },
    );
    
    existingData.add(dataPoint);
    
    if (existingData.length > _maxDataPoints) {
      existingData.removeRange(0, existingData.length - _maxDataPoints);
    }
    
    await _saveDataPoints(_pumpDataKey, existingData);
  }

  // Log system status data
  Future<void> logSystemData(SystemStatus systemStatus) async {
    final prefs = await SharedPreferences.getInstance();
    final existingData = await _getDataPoints(_systemDataKey);
    
    final dataPoint = DataPoint(
      timestamp: DateTime.now(),
      data: systemStatus.toJson(),
    );
    
    existingData.add(dataPoint);
    
    if (existingData.length > _maxDataPoints) {
      existingData.removeRange(0, existingData.length - _maxDataPoints);
    }
    
    await _saveDataPoints(_systemDataKey, existingData);
  }

  // Log alert data
  Future<void> logAlert(Alert alert) async {
    final prefs = await SharedPreferences.getInstance();
    final existingAlerts = await _getAlerts();
    
    existingAlerts.add(alert);
    
    if (existingAlerts.length > _maxAlerts) {
      existingAlerts.removeRange(0, existingAlerts.length - _maxAlerts);
    }
    
    final alertsJson = existingAlerts.map((alert) => alert.toJson()).toList();
    await prefs.setString(_alertDataKey, json.encode(alertsJson));
  }

  // Log water usage
  Future<void> logWaterUsage({
    required double liters,
    required String pumpId,
    required int runtimeMinutes,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    final existingData = await _getDataPoints(_usageDataKey);
    
    final dataPoint = DataPoint(
      timestamp: DateTime.now(),
      data: {
        'liters': liters,
        'pump_id': pumpId,
        'runtime_minutes': runtimeMinutes,
        'efficiency': liters / runtimeMinutes, // liters per minute
      },
    );
    
    existingData.add(dataPoint);
    
    if (existingData.length > _maxDataPoints) {
      existingData.removeRange(0, existingData.length - _maxDataPoints);
    }
    
    await _saveDataPoints(_usageDataKey, existingData);
  }

  // Get tank level history
  Future<List<DataPoint>> getTankHistory({
    DateTime? startDate,
    DateTime? endDate,
    String? tankId,
  }) async {
    final allData = await _getDataPoints(_tankDataKey);
    
    return allData.where((point) {
      if (startDate != null && point.timestamp.isBefore(startDate)) return false;
      if (endDate != null && point.timestamp.isAfter(endDate)) return false;
      
      if (tankId != null) {
        final tanks = point.data['tanks'] as List;
        return tanks.any((tank) => tank['tank_id'] == tankId);
      }
      
      return true;
    }).toList();
  }

  // Get pump history
  Future<List<DataPoint>> getPumpHistory({
    DateTime? startDate,
    DateTime? endDate,
    String? pumpId,
  }) async {
    final allData = await _getDataPoints(_pumpDataKey);
    
    return allData.where((point) {
      if (startDate != null && point.timestamp.isBefore(startDate)) return false;
      if (endDate != null && point.timestamp.isAfter(endDate)) return false;
      
      if (pumpId != null) {
        final pumps = point.data['pumps'] as List;
        return pumps.any((pump) => pump['pump_id'] == pumpId);
      }
      
      return true;
    }).toList();
  }

  // Get system history
  Future<List<DataPoint>> getSystemHistory({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    final allData = await _getDataPoints(_systemDataKey);
    
    return allData.where((point) {
      if (startDate != null && point.timestamp.isBefore(startDate)) return false;
      if (endDate != null && point.timestamp.isAfter(endDate)) return false;
      return true;
    }).toList();
  }

  // Get alert history
  Future<List<Alert>> getAlertHistory({
    DateTime? startDate,
    DateTime? endDate,
    AlertType? type,
  }) async {
    final allAlerts = await _getAlerts();
    
    return allAlerts.where((alert) {
      if (startDate != null && alert.timestamp.isBefore(startDate)) return false;
      if (endDate != null && alert.timestamp.isAfter(endDate)) return false;
      if (type != null && alert.type != type) return false;
      return true;
    }).toList();
  }

  // Get usage statistics
  Future<UsageStatistics> getUsageStatistics({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    final usageData = await _getDataPoints(_usageDataKey);
    final filteredData = usageData.where((point) {
      if (startDate != null && point.timestamp.isBefore(startDate)) return false;
      if (endDate != null && point.timestamp.isAfter(endDate)) return false;
      return true;
    }).toList();

    if (filteredData.isEmpty) {
      return UsageStatistics(
        totalWaterPumped: 0,
        totalPumpRuntime: 0,
        averageDailyUsage: 0,
        peakHourUsage: 0,
        pumpUsageBreakdown: {},
        hourlyUsage: [],
      );
    }

    double totalWater = 0;
    int totalRuntime = 0;
    Map<String, int> pumpBreakdown = {};
    Map<int, double> hourlyUsage = {};

    for (final point in filteredData) {
      final data = point.data;
      final liters = data['liters']?.toDouble() ?? 0.0;
      final runtime = (data['runtime_minutes'] as num?)?.toInt() ?? 0;
      final pumpId = data['pump_id'] as String;
      final hour = point.timestamp.hour;

      totalWater += liters;
      totalRuntime += runtime;
      
      pumpBreakdown[pumpId] = (pumpBreakdown[pumpId] ?? 0) + runtime;
      hourlyUsage[hour] = (hourlyUsage[hour] ?? 0) + liters;
    }

    final days = endDate != null && startDate != null 
        ? endDate.difference(startDate).inDays + 1
        : 1;
    
    final averageDaily = totalWater / days;
    final peakHour = hourlyUsage.values.isNotEmpty 
        ? hourlyUsage.values.reduce((a, b) => a > b ? a : b)
        : 0.0;

    final hourlyDataPoints = hourlyUsage.entries.map((entry) {
      return DataPoint(
        timestamp: DateTime(2024, 1, 1, entry.key),
        data: {'hour': entry.key, 'usage': entry.value},
      );
    }).toList();

    return UsageStatistics(
      totalWaterPumped: totalWater,
      totalPumpRuntime: totalRuntime,
      averageDailyUsage: averageDaily,
      peakHourUsage: peakHour,
      pumpUsageBreakdown: pumpBreakdown,
      hourlyUsage: hourlyDataPoints,
    );
  }

  // Get daily usage for charts
  Future<List<DataPoint>> getDailyUsage({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    final usageData = await _getDataPoints(_usageDataKey);
    final filteredData = usageData.where((point) {
      if (startDate != null && point.timestamp.isBefore(startDate)) return false;
      if (endDate != null && point.timestamp.isAfter(endDate)) return false;
      return true;
    }).toList();

    Map<String, double> dailyUsage = {};
    
    for (final point in filteredData) {
      final dateKey = '${point.timestamp.year}-${point.timestamp.month.toString().padLeft(2, '0')}-${point.timestamp.day.toString().padLeft(2, '0')}';
      final liters = point.data['liters']?.toDouble() ?? 0.0;
      dailyUsage[dateKey] = (dailyUsage[dateKey] ?? 0) + liters;
    }

    return dailyUsage.entries.map((entry) {
      return DataPoint(
        timestamp: DateTime.parse(entry.key),
        data: {'date': entry.key, 'usage': entry.value},
      );
    }).toList()..sort((a, b) => a.timestamp.compareTo(b.timestamp));
  }

  // Clear old data
  Future<void> clearOldData({int daysToKeep = 30}) async {
    final cutoffDate = DateTime.now().subtract(Duration(days: daysToKeep));
    
    await _clearOldDataPoints(_tankDataKey, cutoffDate);
    await _clearOldDataPoints(_pumpDataKey, cutoffDate);
    await _clearOldDataPoints(_systemDataKey, cutoffDate);
    await _clearOldDataPoints(_usageDataKey, cutoffDate);
    await _clearOldAlerts(cutoffDate);
  }

  // Export data as JSON
  Future<Map<String, dynamic>> exportData({
    DateTime? startDate,
    DateTime? endDate,
  }) async {
    return {
      'tank_data': await getTankHistory(startDate: startDate, endDate: endDate),
      'pump_data': await getPumpHistory(startDate: startDate, endDate: endDate),
      'system_data': await getSystemHistory(startDate: startDate, endDate: endDate),
      'alert_data': await getAlertHistory(startDate: startDate, endDate: endDate),
      'usage_statistics': await getUsageStatistics(startDate: startDate, endDate: endDate),
      'export_timestamp': DateTime.now().toIso8601String(),
    };
  }

  // Helper methods
  Future<List<DataPoint>> _getDataPoints(String key) async {
    final prefs = await SharedPreferences.getInstance();
    final jsonString = prefs.getString(key);
    
    if (jsonString == null) return [];
    
    final jsonList = json.decode(jsonString) as List;
    return jsonList.map((json) => DataPoint.fromJson(json)).toList();
  }

  Future<void> _saveDataPoints(String key, List<DataPoint> dataPoints) async {
    final prefs = await SharedPreferences.getInstance();
    final jsonList = dataPoints.map((point) => point.toJson()).toList();
    await prefs.setString(key, json.encode(jsonList));
  }

  Future<List<Alert>> _getAlerts() async {
    final prefs = await SharedPreferences.getInstance();
    final jsonString = prefs.getString(_alertDataKey);
    
    if (jsonString == null) return [];
    
    final jsonList = json.decode(jsonString) as List;
    return jsonList.map((json) => Alert.fromJson(json)).toList();
  }

  Future<void> _clearOldDataPoints(String key, DateTime cutoffDate) async {
    final dataPoints = await _getDataPoints(key);
    final filteredData = dataPoints.where((point) => point.timestamp.isAfter(cutoffDate)).toList();
    await _saveDataPoints(key, filteredData);
  }

  Future<void> _clearOldAlerts(DateTime cutoffDate) async {
    final alerts = await _getAlerts();
    final filteredAlerts = alerts.where((alert) => alert.timestamp.isAfter(cutoffDate)).toList();
    
    final prefs = await SharedPreferences.getInstance();
    final alertsJson = filteredAlerts.map((alert) => alert.toJson()).toList();
    await prefs.setString(_alertDataKey, json.encode(alertsJson));
  }
}