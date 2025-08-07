import 'package:flutter/material.dart';
import 'package:syncfusion_flutter_gauges/gauges.dart';
import '../models/pump_status.dart';

class PumpStatusWidget extends StatelessWidget {
  final PumpStatus pump;

  const PumpStatusWidget({
    super.key,
    required this.pump,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header with pump name and status
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  pump.pumpName,
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                ),
                _buildStatusBadge(),
              ],
            ),
            const SizedBox(height: 16),

            // Current Draw Gauge
            _buildCurrentDrawGauge(context),
            const SizedBox(height: 16),

            // Runtime Information
            _buildRuntimeInfo(context),
            const SizedBox(height: 16),

            // Status Indicators
            _buildStatusIndicators(context),
          ],
        ),
      ),
    );
  }

  Widget _buildStatusBadge() {
    Color color;
    IconData icon;
    
    switch (pump.status) {
      case PumpState.running:
        color = Colors.green;
        icon = Icons.play_circle_filled;
        break;
      case PumpState.off:
        color = Colors.grey;
        icon = Icons.stop_circle;
        break;
      case PumpState.fault:
        color = Colors.red;
        icon = Icons.error;
        break;
      case PumpState.dryRun:
        color = Colors.orange;
        icon = Icons.warning;
        break;
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16, color: color),
          const SizedBox(width: 4),
          Text(
            pump.statusDescription,
            style: TextStyle(
              fontSize: 12,
              color: color,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCurrentDrawGauge(BuildContext context) {
    final maxCurrent = pump.pumpId == PumpType.primary ? 8.0 : 6.0; // Different max for different pumps
    
    return Column(
      children: [
        Text(
          'Current Draw',
          style: Theme.of(context).textTheme.bodyMedium?.copyWith(
            fontWeight: FontWeight.w500,
          ),
        ),
        const SizedBox(height: 8),
        SizedBox(
          height: 100,
          child: SfRadialGauge(
            axes: <RadialAxis>[
              RadialAxis(
                minimum: 0,
                maximum: maxCurrent,
                ranges: <GaugeRange>[
                  GaugeRange(
                    startValue: 0,
                    endValue: maxCurrent * 0.3,
                    color: Colors.green,
                    startWidth: 10,
                    endWidth: 10,
                  ),
                  GaugeRange(
                    startValue: maxCurrent * 0.3,
                    endValue: maxCurrent * 0.7,
                    color: Colors.orange,
                    startWidth: 10,
                    endWidth: 10,
                  ),
                  GaugeRange(
                    startValue: maxCurrent * 0.7,
                    endValue: maxCurrent,
                    color: Colors.red,
                    startWidth: 10,
                    endWidth: 10,
                  ),
                ],
                pointers: <GaugePointer>[
                  NeedlePointer(
                    value: pump.currentDraw,
                    needleColor: _getCurrentColor(),
                    knobStyle: KnobStyle(
                      color: _getCurrentColor(),
                    ),
                  ),
                ],
                annotations: <GaugeAnnotation>[
                  GaugeAnnotation(
                    widget: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Text(
                          '${pump.currentDraw.toStringAsFixed(1)}A',
                          style: Theme.of(context).textTheme.titleMedium?.copyWith(
                            fontWeight: FontWeight.bold,
                            color: _getCurrentColor(),
                          ),
                        ),
                        Text(
                          'Current',
                          style: Theme.of(context).textTheme.bodySmall,
                        ),
                      ],
                    ),
                    angle: 90,
                    positionFactor: 0.5,
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildRuntimeInfo(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).cardColor.withOpacity(0.5),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Today:',
                style: Theme.of(context).textTheme.bodyMedium,
              ),
              Text(
                pump.runtimeTodayFormatted,
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
          const SizedBox(height: 4),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Total:',
                style: Theme.of(context).textTheme.bodyMedium,
              ),
              Text(
                pump.totalRuntimeFormatted,
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
          if (pump.lastStarted != null) ...[
            const SizedBox(height: 4),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  'Last Started:',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                Text(
                  _formatLastStarted(pump.lastStarted!),
                  style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    fontWeight: FontWeight.w500,
                  ),
                ),
              ],
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildStatusIndicators(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        _buildIndicatorChip(
          icon: Icons.speed,
          label: 'Efficiency',
          value: '${(pump.efficiencyRating * 100).toInt()}%',
          color: _getEfficiencyColor(),
        ),
        if (pump.maintenanceDue)
          _buildIndicatorChip(
            icon: Icons.build,
            label: 'Maintenance',
            value: 'Due',
            color: Colors.orange,
          ),
        if (pump.hasFault && pump.faultCode != null)
          _buildIndicatorChip(
            icon: Icons.error,
            label: 'Fault',
            value: pump.faultCode!,
            color: Colors.red,
          ),
      ],
    );
  }

  Widget _buildIndicatorChip({
    required IconData icon,
    required String label,
    required String value,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16, color: color),
          const SizedBox(height: 2),
          Text(
            label,
            style: TextStyle(
              fontSize: 10,
              color: color,
              fontWeight: FontWeight.w500,
            ),
          ),
          Text(
            value,
            style: TextStyle(
              fontSize: 12,
              color: color,
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  Color _getCurrentColor() {
    if (pump.status == PumpState.off) {
      return Colors.grey;
    }
    
    final maxCurrent = pump.pumpId == PumpType.primary ? 8.0 : 6.0;
    final ratio = pump.currentDraw / maxCurrent;
    
    if (ratio < 0.3) {
      return Colors.green;
    } else if (ratio < 0.7) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }

  Color _getEfficiencyColor() {
    if (pump.efficiencyRating > 0.8) {
      return Colors.green;
    } else if (pump.efficiencyRating > 0.6) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }

  String _formatLastStarted(DateTime lastStarted) {
    final now = DateTime.now();
    final difference = now.difference(lastStarted);
    
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
}