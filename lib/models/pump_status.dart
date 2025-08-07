enum PumpState {
  off,
  running,
  fault,
  dryRun
}

enum PumpType {
  primary,
  secondary
}

class PumpStatus {
  final PumpType pumpId;
  final PumpState status;
  final double currentDraw;
  final int runtimeToday;
  final int totalRuntime;
  final DateTime? lastStarted;
  final String? faultCode;
  final bool maintenanceDue;

  PumpStatus({
    required this.pumpId,
    required this.status,
    required this.currentDraw,
    required this.runtimeToday,
    required this.totalRuntime,
    this.lastStarted,
    this.faultCode,
    required this.maintenanceDue,
  });

  factory PumpStatus.fromJson(Map<String, dynamic> json) {
    return PumpStatus(
      pumpId: json['pump_id'] == 'primary' ? PumpType.primary : PumpType.secondary,
      status: PumpState.values.firstWhere(
        (e) => e.name == json['status'],
        orElse: () => PumpState.off,
      ),
      currentDraw: json['current_draw']?.toDouble() ?? 0.0,
      runtimeToday: json['runtime_today'] ?? 0,
      totalRuntime: json['total_runtime'] ?? 0,
      lastStarted: json['last_started'] != null 
          ? DateTime.parse(json['last_started']) 
          : null,
      faultCode: json['fault_code'],
      maintenanceDue: json['maintenance_due'] ?? false,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'pump_id': pumpId.name,
      'status': status.name,
      'current_draw': currentDraw,
      'runtime_today': runtimeToday,
      'total_runtime': totalRuntime,
      'last_started': lastStarted?.toIso8601String(),
      'fault_code': faultCode,
      'maintenance_due': maintenanceDue,
    };
  }

  String get statusDescription {
    switch (status) {
      case PumpState.off:
        return 'Off';
      case PumpState.running:
        return 'Running';
      case PumpState.fault:
        return 'Fault';
      case PumpState.dryRun:
        return 'Dry Run';
    }
  }

  String get pumpName {
    return pumpId == PumpType.primary ? 'Primary Pump' : 'Secondary Pump';
  }

  bool get isRunning => status == PumpState.running;
  bool get hasFault => status == PumpState.fault || status == PumpState.dryRun;
  bool get needsAttention => hasFault || maintenanceDue;

  String get runtimeTodayFormatted {
    final hours = runtimeToday ~/ 3600;
    final minutes = (runtimeToday % 3600) ~/ 60;
    return '${hours}h ${minutes}m';
  }

  String get totalRuntimeFormatted {
    final hours = totalRuntime ~/ 3600;
    return '${hours}h';
  }

  double get efficiencyRating {
    if (totalRuntime == 0) return 1.0;
    
    // Simple efficiency calculation based on current draw and runtime
    final expectedCurrent = pumpId == PumpType.primary ? 5.0 : 4.5;
    final currentEfficiency = currentDraw > 0 ? expectedCurrent / currentDraw : 1.0;
    
    return currentEfficiency.clamp(0.0, 1.0);
  }
}