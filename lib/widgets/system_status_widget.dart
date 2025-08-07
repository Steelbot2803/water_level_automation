import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/water_system_provider.dart';
import '../models/system_status.dart';

class SystemStatusWidget extends StatelessWidget {
  const SystemStatusWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<WaterSystemProvider>(
      builder: (context, provider, child) {
        final systemStatus = provider.systemStatus;
        
        if (systemStatus == null) {
          return Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Row(
                children: [
                  const CircularProgressIndicator(),
                  const SizedBox(width: 16),
                  Text(
                    'Loading system status...',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                ],
              ),
            ),
          );
        }

        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'System Status',
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    _buildConnectionIndicator(provider.isConnected),
                  ],
                ),
                const SizedBox(height: 16),
                
                // Status Grid
                Row(
                  children: [
                    Expanded(
                      child: _buildStatusItem(
                        context,
                        icon: _getModeIcon(systemStatus.mode),
                        label: 'Mode',
                        value: systemStatus.modeDescription,
                        color: _getModeColor(systemStatus.mode),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: _buildStatusItem(
                        context,
                        icon: Icons.water_drop,
                        label: 'Active Pump',
                        value: systemStatus.activePumpDescription,
                        color: systemStatus.activePump == ActivePump.none 
                            ? Colors.grey 
                            : Colors.blue,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                
                Row(
                  children: [
                    Expanded(
                      child: _buildStatusItem(
                        context,
                        icon: Icons.access_time,
                        label: 'Uptime',
                        value: systemStatus.uptimeFormatted,
                        color: Colors.green,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: _buildStatusItem(
                        context,
                        icon: Icons.notifications,
                        label: 'Alerts',
                        value: '${systemStatus.alertsCount}',
                        color: systemStatus.alertsCount > 0 
                            ? Colors.red 
                            : Colors.green,
                      ),
                    ),
                  ],
                ),
                
                // Next Scheduled Operation
                if (systemStatus.hasScheduledOperation) ...[
                  const SizedBox(height: 16),
                  Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      color: Colors.blue.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(8),
                      border: Border.all(color: Colors.blue.withOpacity(0.3)),
                    ),
                    child: Row(
                      children: [
                        Icon(Icons.schedule, color: Colors.blue, size: 20),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                'Next Scheduled Operation',
                                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                  color: Colors.blue,
                                  fontWeight: FontWeight.w500,
                                ),
                              ),
                              Text(
                                _formatNextScheduled(systemStatus.nextScheduled!),
                                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                                  color: Colors.blue,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
                
                // Error Message
                if (provider.errorMessage != null) ...[
                  const SizedBox(height: 16),
                  Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      color: Colors.red.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(8),
                      border: Border.all(color: Colors.red.withOpacity(0.3)),
                    ),
                    child: Row(
                      children: [
                        Icon(Icons.error, color: Colors.red, size: 20),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Text(
                            provider.errorMessage!,
                            style: TextStyle(color: Colors.red),
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildConnectionIndicator(bool isConnected) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: isConnected ? Colors.green.withOpacity(0.1) : Colors.red.withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: isConnected ? Colors.green : Colors.red,
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            isConnected ? Icons.wifi : Icons.wifi_off,
            size: 16,
            color: isConnected ? Colors.green : Colors.red,
          ),
          const SizedBox(width: 4),
          Text(
            isConnected ? 'Connected' : 'Disconnected',
            style: TextStyle(
              fontSize: 12,
              color: isConnected ? Colors.green : Colors.red,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusItem(
    BuildContext context, {
    required IconData icon,
    required String label,
    required String value,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, size: 16, color: color),
              const SizedBox(width: 4),
              Text(
                label,
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: color,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ],
          ),
          const SizedBox(height: 4),
          Text(
            value,
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
              color: color,
              fontWeight: FontWeight.bold,
            ),
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
          ),
        ],
      ),
    );
  }

  IconData _getModeIcon(SystemMode mode) {
    switch (mode) {
      case SystemMode.auto:
        return Icons.auto_mode;
      case SystemMode.manual:
        return Icons.touch_app;
      case SystemMode.scheduled:
        return Icons.schedule;
      case SystemMode.emergency:
        return Icons.emergency;
    }
  }

  Color _getModeColor(SystemMode mode) {
    switch (mode) {
      case SystemMode.auto:
        return Colors.green;
      case SystemMode.manual:
        return Colors.blue;
      case SystemMode.scheduled:
        return Colors.purple;
      case SystemMode.emergency:
        return Colors.red;
    }
  }

  String _formatNextScheduled(DateTime nextScheduled) {
    final now = DateTime.now();
    final difference = nextScheduled.difference(now);
    
    if (difference.inMinutes < 1) {
      return 'Starting now';
    } else if (difference.inHours < 1) {
      return 'In ${difference.inMinutes} minutes';
    } else if (difference.inDays < 1) {
      return 'In ${difference.inHours}h ${difference.inMinutes % 60}m';
    } else {
      return 'In ${difference.inDays} days';
    }
  }
}