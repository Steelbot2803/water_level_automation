enum SystemMode {
  auto,
  manual,
  scheduled,
  emergency
}

enum ActivePump {
  none,
  primary,
  secondary
}

class SystemStatus {
  final SystemMode mode;
  final ActivePump activePump;
  final bool connectivity;
  final int uptime;
  final int alertsCount;
  final DateTime? nextScheduled;

  SystemStatus({
    required this.mode,
    required this.activePump,
    required this.connectivity,
    required this.uptime,
    required this.alertsCount,
    this.nextScheduled,
  });

  factory SystemStatus.fromJson(Map<String, dynamic> json) {
    return SystemStatus(
      mode: SystemMode.values.firstWhere(
        (e) => e.name == json['mode'],
        orElse: () => SystemMode.auto,
      ),
      activePump: ActivePump.values.firstWhere(
        (e) => e.name == json['active_pump'],
        orElse: () => ActivePump.none,
      ),
      connectivity: json['connectivity'] ?? false,
      uptime: json['uptime'] ?? 0,
      alertsCount: json['alerts_count'] ?? 0,
      nextScheduled: json['next_scheduled'] != null
          ? DateTime.parse(json['next_scheduled'])
          : null,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'mode': mode.name,
      'active_pump': activePump.name,
      'connectivity': connectivity,
      'uptime': uptime,
      'alerts_count': alertsCount,
      'next_scheduled': nextScheduled?.toIso8601String(),
    };
  }

  String get modeDescription {
    switch (mode) {
      case SystemMode.auto:
        return 'Automatic';
      case SystemMode.manual:
        return 'Manual Override';
      case SystemMode.scheduled:
        return 'Scheduled';
      case SystemMode.emergency:
        return 'Emergency Stop';
    }
  }

  String get activePumpDescription {
    switch (activePump) {
      case ActivePump.none:
        return 'No pump active';
      case ActivePump.primary:
        return 'Primary pump active';
      case ActivePump.secondary:
        return 'Secondary pump active';
    }
  }

  String get uptimeFormatted {
    final days = uptime ~/ 86400;
    final hours = (uptime % 86400) ~/ 3600;
    final minutes = (uptime % 3600) ~/ 60;
    
    if (days > 0) {
      return '${days}d ${hours}h ${minutes}m';
    } else if (hours > 0) {
      return '${hours}h ${minutes}m';
    } else {
      return '${minutes}m';
    }
  }

  bool get hasAlerts => alertsCount > 0;
  bool get isOperational => connectivity && mode != SystemMode.emergency;
  bool get hasScheduledOperation => nextScheduled != null;
}