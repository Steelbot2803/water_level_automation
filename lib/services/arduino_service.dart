import 'dart:convert';
import 'dart:async';
import 'package:http/http.dart' as http;
import '../models/tank_level.dart';
import '../models/pump_status.dart';
import '../models/alert.dart';
import '../models/schedule.dart';

class ArduinoService {
  static const String defaultIp = 'http://192.168.1.197';
  String _baseUrl = defaultIp;
  Timer? _statusTimer;
  final StreamController<Map<String, dynamic>> _statusController = 
      StreamController<Map<String, dynamic>>.broadcast();

  // Getters
  String get baseUrl => _baseUrl;
  Stream<Map<String, dynamic>> get statusStream => _statusController.stream;

  void setBaseUrl(String url) {
    _baseUrl = url.startsWith('http') ? url : 'http://$url';
  }

  // Status & Monitoring
  Future<Map<String, dynamic>> getSystemStatus() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/status'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        _statusController.add(data);
        return data;
      } else {
        throw Exception('Failed to get status: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  Future<List<TankLevel>> getTankLevels() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/tanks'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return (data['tanks'] as List)
            .map((tank) => TankLevel.fromJson(tank))
            .toList();
      } else {
        throw Exception('Failed to get tank levels: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  Future<List<PumpStatus>> getPumpStatus() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/pumps'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return (data['pumps'] as List)
            .map((pump) => PumpStatus.fromJson(pump))
            .toList();
      } else {
        throw Exception('Failed to get pump status: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  Future<List<Alert>> getAlerts() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/alerts'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return (data['alerts'] as List)
            .map((alert) => Alert.fromJson(alert))
            .toList();
      } else {
        throw Exception('Failed to get alerts: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  // Pump Control
  Future<bool> startPump(String pumpId) async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/pump/start/$pumpId'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to start pump: $e');
    }
  }

  Future<bool> stopPump() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/pump/stop'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to stop pump: $e');
    }
  }

  Future<bool> switchPump() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/pump/switch'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to switch pump: $e');
    }
  }

  Future<bool> testPump(String pumpId) async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/pump/test/$pumpId'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to test pump: $e');
    }
  }

  // System Modes
  Future<bool> setAutoMode() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/mode/auto'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to set auto mode: $e');
    }
  }

  Future<bool> setManualMode() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/mode/manual'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to set manual mode: $e');
    }
  }

  Future<bool> setScheduledMode() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/mode/scheduled'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to set scheduled mode: $e');
    }
  }

  Future<bool> emergencyStop() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/mode/emergency'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to emergency stop: $e');
    }
  }

  // Scheduling
  Future<bool> setSchedule(Schedule schedule) async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/schedule/set'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode(schedule.toJson()),
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to set schedule: $e');
    }
  }

  Future<List<Schedule>> getSchedules() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/schedule'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        return (data['schedules'] as List)
            .map((schedule) => Schedule.fromJson(schedule))
            .toList();
      } else {
        throw Exception('Failed to get schedules: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  Future<bool> deleteSchedule(String scheduleId) async {
    try {
      final response = await http.delete(
        Uri.parse('$_baseUrl/schedule/$scheduleId'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to delete schedule: $e');
    }
  }

  Future<bool> enableSchedule(String scheduleId, bool enabled) async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/schedule/enable/$scheduleId'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode({'enabled': enabled}),
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to enable/disable schedule: $e');
    }
  }

  // Configuration
  Future<bool> setConfiguration(Map<String, dynamic> config) async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/config'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode(config),
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to set configuration: $e');
    }
  }

  Future<Map<String, dynamic>> getConfiguration() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/config'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        return json.decode(response.body);
      } else {
        throw Exception('Failed to get configuration: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Connection error: $e');
    }
  }

  Future<bool> resetSystem() async {
    try {
      final response = await http.post(
        Uri.parse('$_baseUrl/reset'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      return response.statusCode == 200;
    } catch (e) {
      throw Exception('Failed to reset system: $e');
    }
  }

  // Auto-refresh functionality
  void startAutoRefresh({Duration interval = const Duration(seconds: 5)}) {
    _statusTimer?.cancel();
    _statusTimer = Timer.periodic(interval, (timer) {
      getSystemStatus().catchError((error) {
        // Handle error silently for auto-refresh
        return <String, dynamic>{};
      });
    });
  }

  void stopAutoRefresh() {
    _statusTimer?.cancel();
    _statusTimer = null;
  }

  // Connection test
  Future<bool> testConnection() async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/status'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 5));

      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }

  void dispose() {
    _statusTimer?.cancel();
    _statusController.close();
  }
}