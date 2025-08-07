import 'package:flutter/material.dart';
import 'package:syncfusion_flutter_gauges/gauges.dart';
import '../models/tank_level.dart';

class TankLevelWidget extends StatelessWidget {
  final TankLevel tank;
  final String title;

  const TankLevelWidget({
    super.key,
    required this.tank,
    required this.title,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Text(
              title,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            
            // Discrete Level Indicator
            _buildDiscreteLevelIndicator(),
            const SizedBox(height: 16),
            
            // Circular Gauge
            SizedBox(
              height: 120,
              child: SfRadialGauge(
                axes: <RadialAxis>[
                  RadialAxis(
                    minimum: 0,
                    maximum: 100,
                    showLabels: false,
                    showTicks: false,
                    axisLineStyle: const AxisLineStyle(
                      thickness: 0.2,
                      cornerStyle: CornerStyle.bothCurve,
                      color: Color.fromARGB(30, 0, 169, 181),
                      thicknessUnit: GaugeSizeUnit.factor,
                    ),
                    pointers: <GaugePointer>[
                      RangePointer(
                        value: tank.levelPercentage,
                        width: 0.2,
                        sizeUnit: GaugeSizeUnit.factor,
                        cornerStyle: CornerStyle.bothCurve,
                        color: _getLevelColor(),
                      ),
                    ],
                    annotations: <GaugeAnnotation>[
                      GaugeAnnotation(
                        positionFactor: 0.1,
                        angle: 90,
                        widget: Column(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Text(
                              '${tank.levelPercentage.toInt()}%',
                              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                                fontWeight: FontWeight.bold,
                                color: _getLevelColor(),
                              ),
                            ),
                            Text(
                              tank.levelDescription,
                              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                color: _getLevelColor(),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Status indicators
            _buildStatusIndicators(),
          ],
        ),
      ),
    );
  }

  Widget _buildDiscreteLevelIndicator() {
    return Container(
      height: 120,
      width: 40,
      decoration: BoxDecoration(
        border: Border.all(color: Colors.grey),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        children: [
          _buildLevelSegment(WaterLevel.full, 'Full'),
          _buildLevelSegment(WaterLevel.high, 'High'),
          _buildLevelSegment(WaterLevel.medium, 'Med'),
          _buildLevelSegment(WaterLevel.low, 'Low'),
          _buildLevelSegment(WaterLevel.empty, 'Empty'),
        ],
      ),
    );
  }

  Widget _buildLevelSegment(WaterLevel level, String label) {
    final isActive = tank.level.index >= level.index;
    final color = isActive ? _getLevelColorForLevel(level) : Colors.grey.withOpacity(0.3);
    
    return Expanded(
      child: Container(
        margin: const EdgeInsets.all(1),
        decoration: BoxDecoration(
          color: color,
          borderRadius: BorderRadius.circular(4),
        ),
        child: Center(
          child: Text(
            label,
            style: TextStyle(
              fontSize: 10,
              fontWeight: FontWeight.bold,
              color: isActive ? Colors.white : Colors.grey,
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildStatusIndicators() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        _buildStatusChip(
          icon: Icons.sensors,
          label: 'Sensors',
          color: _getSensorStatusColor(),
        ),
        _buildStatusChip(
          icon: _getStatusIcon(),
          label: _getStatusText(),
          color: _getLevelColor(),
        ),
      ],
    );
  }

  Widget _buildStatusChip({
    required IconData icon,
    required String label,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16, color: color),
          const SizedBox(width: 4),
          Text(
            label,
            style: TextStyle(
              fontSize: 12,
              color: color,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Color _getLevelColor() {
    switch (tank.level) {
      case WaterLevel.empty:
        return Colors.red;
      case WaterLevel.low:
        return Colors.orange;
      case WaterLevel.medium:
        return Colors.yellow;
      case WaterLevel.high:
        return Colors.lightGreen;
      case WaterLevel.full:
        return Colors.green;
    }
  }

  Color _getLevelColorForLevel(WaterLevel level) {
    switch (level) {
      case WaterLevel.empty:
        return Colors.red;
      case WaterLevel.low:
        return Colors.orange;
      case WaterLevel.medium:
        return Colors.yellow;
      case WaterLevel.high:
        return Colors.lightGreen;
      case WaterLevel.full:
        return Colors.green;
    }
  }

  Color _getSensorStatusColor() {
    // Check if all expected sensors are working
    final expectedSensors = ['empty', 'low', 'medium', 'high', 'full'];
    final workingSensors = tank.sensors.values.where((working) => working).length;
    
    if (workingSensors == expectedSensors.length) {
      return Colors.green;
    } else if (workingSensors > expectedSensors.length / 2) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }

  IconData _getStatusIcon() {
    if (tank.isCritical) {
      return Icons.error;
    } else if (tank.isWarning) {
      return Icons.warning;
    } else if (tank.isOverflow) {
      return Icons.warning;
    } else {
      return Icons.check_circle;
    }
  }

  String _getStatusText() {
    if (tank.isCritical) {
      return 'Critical';
    } else if (tank.isWarning) {
      return 'Warning';
    } else if (tank.isOverflow) {
      return 'Overflow Risk';
    } else {
      return 'Normal';
    }
  }
}