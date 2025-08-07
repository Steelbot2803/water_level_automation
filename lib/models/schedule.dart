import 'package:intl/intl.dart';

enum ScheduleDay {
  monday,
  tuesday,
  wednesday,
  thursday,
  friday,
  saturday,
  sunday
}

enum PumpPreference {
  primary,
  secondary,
  auto
}

class ScheduleConditions {
  final String minSecondaryLevel;
  final String maxOverheadLevel;

  ScheduleConditions({
    required this.minSecondaryLevel,
    required this.maxOverheadLevel,
  });

  factory ScheduleConditions.fromJson(Map<String, dynamic> json) {
    return ScheduleConditions(
      minSecondaryLevel: json['min_secondary_level'] ?? 'low',
      maxOverheadLevel: json['max_overhead_level'] ?? 'high',
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'min_secondary_level': minSecondaryLevel,
      'max_overhead_level': maxOverheadLevel,
    };
  }
}

class Schedule {
  final String id;
  final String name;
  final bool enabled;
  final List<ScheduleDay> days;
  final String startTime; // HH:MM format
  final int duration; // in minutes
  final PumpPreference pumpPreference;
  final ScheduleConditions conditions;

  Schedule({
    required this.id,
    required this.name,
    required this.enabled,
    required this.days,
    required this.startTime,
    required this.duration,
    required this.pumpPreference,
    required this.conditions,
  });

  factory Schedule.fromJson(Map<String, dynamic> json) {
    return Schedule(
      id: json['id'],
      name: json['name'],
      enabled: json['enabled'] ?? true,
      days: (json['days'] as List<dynamic>)
          .map((day) => ScheduleDay.values.firstWhere(
                (e) => e.name == day,
                orElse: () => ScheduleDay.monday,
              ))
          .toList(),
      startTime: json['start_time'],
      duration: json['duration'],
      pumpPreference: PumpPreference.values.firstWhere(
        (e) => e.name == json['pump_preference'],
        orElse: () => PumpPreference.auto,
      ),
      conditions: ScheduleConditions.fromJson(json['conditions'] ?? {}),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'name': name,
      'enabled': enabled,
      'days': days.map((day) => day.name).toList(),
      'start_time': startTime,
      'duration': duration,
      'pump_preference': pumpPreference.name,
      'conditions': conditions.toJson(),
    };
  }

  String get daysDescription {
    if (days.length == 7) {
      return 'Every day';
    } else if (days.length == 5 && 
               days.contains(ScheduleDay.monday) &&
               days.contains(ScheduleDay.tuesday) &&
               days.contains(ScheduleDay.wednesday) &&
               days.contains(ScheduleDay.thursday) &&
               days.contains(ScheduleDay.friday)) {
      return 'Weekdays';
    } else if (days.length == 2 &&
               days.contains(ScheduleDay.saturday) &&
               days.contains(ScheduleDay.sunday)) {
      return 'Weekends';
    } else {
      return days.map((day) => _dayAbbreviation(day)).join(', ');
    }
  }

  String _dayAbbreviation(ScheduleDay day) {
    switch (day) {
      case ScheduleDay.monday:
        return 'Mon';
      case ScheduleDay.tuesday:
        return 'Tue';
      case ScheduleDay.wednesday:
        return 'Wed';
      case ScheduleDay.thursday:
        return 'Thu';
      case ScheduleDay.friday:
        return 'Fri';
      case ScheduleDay.saturday:
        return 'Sat';
      case ScheduleDay.sunday:
        return 'Sun';
    }
  }

  String get durationFormatted {
    final hours = duration ~/ 60;
    final minutes = duration % 60;
    
    if (hours > 0 && minutes > 0) {
      return '${hours}h ${minutes}m';
    } else if (hours > 0) {
      return '${hours}h';
    } else {
      return '${minutes}m';
    }
  }

  String get pumpPreferenceDescription {
    switch (pumpPreference) {
      case PumpPreference.primary:
        return 'Primary Pump';
      case PumpPreference.secondary:
        return 'Secondary Pump';
      case PumpPreference.auto:
        return 'Auto Select';
    }
  }

  String get startTimeFormatted {
    try {
      final time = DateFormat('HH:mm').parse(startTime);
      return DateFormat('h:mm a').format(time);
    } catch (e) {
      return startTime;
    }
  }

  bool get isActiveToday {
    final today = DateTime.now().weekday;
    final todayScheduleDay = ScheduleDay.values[today - 1];
    return enabled && days.contains(todayScheduleDay);
  }

  DateTime? get nextExecution {
    if (!enabled) return null;
    
    final now = DateTime.now();
    final timeParts = startTime.split(':');
    final hour = int.parse(timeParts[0]);
    final minute = int.parse(timeParts[1]);
    
    for (int i = 0; i < 7; i++) {
      final checkDate = now.add(Duration(days: i));
      final scheduleDay = ScheduleDay.values[checkDate.weekday - 1];
      
      if (days.contains(scheduleDay)) {
        final executionTime = DateTime(
          checkDate.year,
          checkDate.month,
          checkDate.day,
          hour,
          minute,
        );
        
        if (executionTime.isAfter(now)) {
          return executionTime;
        }
      }
    }
    
    return null;
  }

  Schedule copyWith({
    String? id,
    String? name,
    bool? enabled,
    List<ScheduleDay>? days,
    String? startTime,
    int? duration,
    PumpPreference? pumpPreference,
    ScheduleConditions? conditions,
  }) {
    return Schedule(
      id: id ?? this.id,
      name: name ?? this.name,
      enabled: enabled ?? this.enabled,
      days: days ?? this.days,
      startTime: startTime ?? this.startTime,
      duration: duration ?? this.duration,
      pumpPreference: pumpPreference ?? this.pumpPreference,
      conditions: conditions ?? this.conditions,
    );
  }
}