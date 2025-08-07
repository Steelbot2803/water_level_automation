enum WaterLevel {
  empty,
  low,
  medium,
  high,
  full
}

class TankLevel {
  final String tankId;
  final WaterLevel level;
  final int levelNumeric;
  final DateTime timestamp;
  final Map<String, bool> sensors;

  TankLevel({
    required this.tankId,
    required this.level,
    required this.levelNumeric,
    required this.timestamp,
    required this.sensors,
  });

  factory TankLevel.fromJson(Map<String, dynamic> json) {
    return TankLevel(
      tankId: json['tank_id'],
      level: WaterLevel.values[json['level_numeric']],
      levelNumeric: json['level_numeric'],
      timestamp: DateTime.parse(json['timestamp']),
      sensors: Map<String, bool>.from(json['sensors']),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'tank_id': tankId,
      'level': level.name,
      'level_numeric': levelNumeric,
      'timestamp': timestamp.toIso8601String(),
      'sensors': sensors,
    };
  }

  double get levelPercentage {
    switch (level) {
      case WaterLevel.empty:
        return 0.0;
      case WaterLevel.low:
        return 25.0;
      case WaterLevel.medium:
        return 50.0;
      case WaterLevel.high:
        return 75.0;
      case WaterLevel.full:
        return 100.0;
    }
  }

  String get levelDescription {
    switch (level) {
      case WaterLevel.empty:
        return 'Empty';
      case WaterLevel.low:
        return 'Low';
      case WaterLevel.medium:
        return 'Medium';
      case WaterLevel.high:
        return 'High';
      case WaterLevel.full:
        return 'Full';
    }
  }

  bool get isCritical => level == WaterLevel.empty;
  bool get isWarning => level == WaterLevel.low;
  bool get isOverflow => level == WaterLevel.full && tankId == 'overhead';
}