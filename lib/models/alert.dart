enum AlertType {
  critical,
  warning,
  info
}

enum AlertCategory {
  tankEmpty,
  tankLow,
  tankOverflow,
  pumpFault,
  pumpDryRun,
  communicationLoss,
  maintenanceDue,
  scheduleComplete,
  systemModeChange,
  emergencyStop
}

class Alert {
  final String id;
  final AlertType type;
  final AlertCategory category;
  final String title;
  final String message;
  final DateTime timestamp;
  final bool acknowledged;
  final Map<String, dynamic>? metadata;

  Alert({
    required this.id,
    required this.type,
    required this.category,
    required this.title,
    required this.message,
    required this.timestamp,
    this.acknowledged = false,
    this.metadata,
  });

  factory Alert.fromJson(Map<String, dynamic> json) {
    return Alert(
      id: json['id'],
      type: AlertType.values.firstWhere(
        (e) => e.name == json['type'],
        orElse: () => AlertType.info,
      ),
      category: AlertCategory.values.firstWhere(
        (e) => e.name == json['category'],
        orElse: () => AlertCategory.systemModeChange,
      ),
      title: json['title'],
      message: json['message'],
      timestamp: DateTime.parse(json['timestamp']),
      acknowledged: json['acknowledged'] ?? false,
      metadata: json['metadata'],
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'type': type.name,
      'category': category.name,
      'title': title,
      'message': message,
      'timestamp': timestamp.toIso8601String(),
      'acknowledged': acknowledged,
      'metadata': metadata,
    };
  }

  String get typeDescription {
    switch (type) {
      case AlertType.critical:
        return 'Critical';
      case AlertType.warning:
        return 'Warning';
      case AlertType.info:
        return 'Info';
    }
  }

  String get categoryDescription {
    switch (category) {
      case AlertCategory.tankEmpty:
        return 'Tank Empty';
      case AlertCategory.tankLow:
        return 'Tank Low';
      case AlertCategory.tankOverflow:
        return 'Tank Overflow';
      case AlertCategory.pumpFault:
        return 'Pump Fault';
      case AlertCategory.pumpDryRun:
        return 'Pump Dry Run';
      case AlertCategory.communicationLoss:
        return 'Communication Loss';
      case AlertCategory.maintenanceDue:
        return 'Maintenance Due';
      case AlertCategory.scheduleComplete:
        return 'Schedule Complete';
      case AlertCategory.systemModeChange:
        return 'System Mode Change';
      case AlertCategory.emergencyStop:
        return 'Emergency Stop';
    }
  }

  bool get requiresImmediateAction {
    return type == AlertType.critical || 
           category == AlertCategory.emergencyStop ||
           category == AlertCategory.pumpFault ||
           category == AlertCategory.communicationLoss;
  }

  String get formattedTimestamp {
    final now = DateTime.now();
    final difference = now.difference(timestamp);
    
    if (difference.inMinutes < 1) {
      return 'Just now';
    } else if (difference.inHours < 1) {
      return '${difference.inMinutes}m ago';
    } else if (difference.inDays < 1) {
      return '${difference.inHours}h ago';
    } else {
      return '${difference.inDays}d ago';
    }
  }

  Alert copyWith({
    String? id,
    AlertType? type,
    AlertCategory? category,
    String? title,
    String? message,
    DateTime? timestamp,
    bool? acknowledged,
    Map<String, dynamic>? metadata,
  }) {
    return Alert(
      id: id ?? this.id,
      type: type ?? this.type,
      category: category ?? this.category,
      title: title ?? this.title,
      message: message ?? this.message,
      timestamp: timestamp ?? this.timestamp,
      acknowledged: acknowledged ?? this.acknowledged,
      metadata: metadata ?? this.metadata,
    );
  }
}