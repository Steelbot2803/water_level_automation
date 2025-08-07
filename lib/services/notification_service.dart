import 'dart:async';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/alert.dart';

class NotificationService {
  static final NotificationService _instance = NotificationService._internal();
  factory NotificationService() => _instance;
  NotificationService._internal();

  final FlutterLocalNotificationsPlugin _localNotifications =
      FlutterLocalNotificationsPlugin();
  
  bool _initialized = false;
  StreamController<Alert>? _alertController;

  Stream<Alert> get alertStream => _alertController?.stream ?? const Stream.empty();

  Future<void> initialize() async {
    if (_initialized) return;

    _alertController = StreamController<Alert>.broadcast();

    // Initialize local notifications
    const androidSettings = AndroidInitializationSettings('@mipmap/ic_launcher');
    const iosSettings = DarwinInitializationSettings(
      requestAlertPermission: true,
      requestBadgePermission: true,
      requestSoundPermission: true,
    );
    
    const initSettings = InitializationSettings(
      android: androidSettings,
      iOS: iosSettings,
    );

    await _localNotifications.initialize(
      initSettings,
      onDidReceiveNotificationResponse: _onNotificationTapped,
    );

    // Firebase messaging removed for web compatibility

    _initialized = true;
  }

  // Firebase messaging methods removed for web compatibility

  void _onNotificationTapped(NotificationResponse response) {
    print('Local notification tapped: ${response.payload}');
    // Handle local notification tap
  }

  Future<void> _showLocalNotification({
    required String title,
    required String body,
    String? payload,
    int id = 0,
  }) async {
    const androidDetails = AndroidNotificationDetails(
      'water_level_alerts',
      'Water Level Alerts',
      channelDescription: 'Notifications for water level system alerts',
      importance: Importance.high,
      priority: Priority.high,
      showWhen: true,
      enableVibration: true,
      playSound: true,
    );

    const iosDetails = DarwinNotificationDetails(
      presentAlert: true,
      presentBadge: true,
      presentSound: true,
    );

    const notificationDetails = NotificationDetails(
      android: androidDetails,
      iOS: iosDetails,
    );

    await _localNotifications.show(
      id,
      title,
      body,
      notificationDetails,
      payload: payload,
    );
  }

  Future<void> showAlertNotification(Alert alert) async {
    String title;
    String body;
    int notificationId;

    switch (alert.type) {
      case AlertType.critical:
        title = '🚨 Critical Alert';
        notificationId = 1000 + alert.hashCode % 1000;
        break;
      case AlertType.warning:
        title = '⚠️ Warning';
        notificationId = 2000 + alert.hashCode % 1000;
        break;
      case AlertType.info:
        title = 'ℹ️ Information';
        notificationId = 3000 + alert.hashCode % 1000;
        break;
    }

    body = '${alert.categoryDescription}: ${alert.message}';

    await _showLocalNotification(
      id: notificationId,
      title: title,
      body: body,
      payload: alert.id,
    );

    // Add to alert stream
    _alertController?.add(alert);
  }

  Future<void> showSystemNotification({
    required String title,
    required String message,
    AlertType type = AlertType.info,
  }) async {
    final alert = Alert(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      type: type,
      category: AlertCategory.systemModeChange,
      title: title,
      message: message,
      timestamp: DateTime.now(),
    );

    await showAlertNotification(alert);
  }

  Future<void> showTankAlert({
    required String tankName,
    required String level,
    required AlertType type,
  }) async {
    AlertCategory category;
    String message;

    switch (level.toLowerCase()) {
      case 'empty':
        category = AlertCategory.tankEmpty;
        message = '$tankName is empty!';
        break;
      case 'low':
        category = AlertCategory.tankLow;
        message = '$tankName level is low';
        break;
      case 'full':
        category = AlertCategory.tankOverflow;
        message = '$tankName is full - risk of overflow';
        break;
      default:
        category = AlertCategory.tankLow;
        message = '$tankName level: $level';
    }

    final alert = Alert(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      type: type,
      category: category,
      title: 'Tank Level Alert',
      message: message,
      timestamp: DateTime.now(),
      metadata: {
        'tank': tankName,
        'level': level,
      },
    );

    await showAlertNotification(alert);
  }

  Future<void> showPumpAlert({
    required String pumpName,
    required String status,
    required AlertType type,
    String? faultCode,
  }) async {
    AlertCategory category;
    String message;

    switch (status.toLowerCase()) {
      case 'fault':
        category = AlertCategory.pumpFault;
        message = '$pumpName has a fault${faultCode != null ? ': $faultCode' : ''}';
        break;
      case 'dry_run':
      case 'dryrun':
        category = AlertCategory.pumpDryRun;
        message = '$pumpName is running dry - check water supply';
        break;
      default:
        category = AlertCategory.pumpFault;
        message = '$pumpName status: $status';
    }

    final alert = Alert(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      type: type,
      category: category,
      title: 'Pump Alert',
      message: message,
      timestamp: DateTime.now(),
      metadata: {
        'pump': pumpName,
        'status': status,
        'fault_code': faultCode,
      },
    );

    await showAlertNotification(alert);
  }

  Future<void> showMaintenanceAlert({
    required String pumpName,
    required int totalHours,
  }) async {
    final alert = Alert(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      type: AlertType.warning,
      category: AlertCategory.maintenanceDue,
      title: 'Maintenance Due',
      message: '$pumpName has run for ${totalHours}h - maintenance recommended',
      timestamp: DateTime.now(),
      metadata: {
        'pump': pumpName,
        'total_hours': totalHours,
      },
    );

    await showAlertNotification(alert);
  }

  Future<void> showScheduleNotification({
    required String scheduleName,
    required String action,
  }) async {
    final alert = Alert(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      type: AlertType.info,
      category: AlertCategory.scheduleComplete,
      title: 'Schedule $action',
      message: 'Schedule "$scheduleName" has $action',
      timestamp: DateTime.now(),
      metadata: {
        'schedule': scheduleName,
        'action': action,
      },
    );

    await showAlertNotification(alert);
  }

  Future<void> cancelNotification(int id) async {
    await _localNotifications.cancel(id);
  }

  Future<void> cancelAllNotifications() async {
    await _localNotifications.cancelAll();
  }

  Future<String?> getFCMToken() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString('fcm_token');
  }

  Future<bool> areNotificationsEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool('notifications_enabled') ?? true;
  }

  Future<void> setNotificationsEnabled(bool enabled) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('notifications_enabled', enabled);
  }

  Future<bool> areCriticalAlertsEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool('critical_alerts_enabled') ?? true;
  }

  Future<void> setCriticalAlertsEnabled(bool enabled) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('critical_alerts_enabled', enabled);
  }

  Future<bool> areWarningAlertsEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool('warning_alerts_enabled') ?? true;
  }

  Future<void> setWarningAlertsEnabled(bool enabled) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('warning_alerts_enabled', enabled);
  }

  Future<bool> areInfoAlertsEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool('info_alerts_enabled') ?? false;
  }

  Future<void> setInfoAlertsEnabled(bool enabled) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('info_alerts_enabled', enabled);
  }

  void dispose() {
    _alertController?.close();
  }
}