import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/water_system_provider.dart';
import '../models/pump_status.dart';

class QuickActionsWidget extends StatelessWidget {
  const QuickActionsWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<WaterSystemProvider>(
      builder: (context, provider, child) {
        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Quick Actions',
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 16),
                
                // Emergency Actions Row
                Row(
                  children: [
                    Expanded(
                      child: _buildActionButton(
                        context,
                        icon: Icons.emergency,
                        label: 'Emergency Stop',
                        color: Colors.red,
                        onPressed: () => _showEmergencyStopDialog(context, provider),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: _buildActionButton(
                        context,
                        icon: Icons.refresh,
                        label: 'Refresh Data',
                        color: Colors.blue,
                        onPressed: provider.isLoading 
                            ? null 
                            : () => provider.refreshData(),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                
                // Pump Control Row
                Row(
                  children: [
                    Expanded(
                      child: _buildActionButton(
                        context,
                        icon: provider.hasActivePump ? Icons.stop : Icons.play_arrow,
                        label: provider.hasActivePump ? 'Stop Pumps' : 'Start Primary',
                        color: provider.hasActivePump ? Colors.orange : Colors.green,
                        onPressed: () => provider.hasActivePump 
                            ? _showStopPumpDialog(context, provider)
                            : _showStartPumpDialog(context, provider),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: _buildActionButton(
                        context,
                        icon: Icons.swap_horiz,
                        label: 'Switch Pump',
                        color: Colors.purple,
                        onPressed: provider.hasActivePump 
                            ? () => _showSwitchPumpDialog(context, provider)
                            : null,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                
                // Mode Control Row
                Row(
                  children: [
                    Expanded(
                      child: _buildModeButton(
                        context,
                        icon: Icons.auto_mode,
                        label: 'Auto Mode',
                        isActive: provider.systemStatus?.mode.name == 'auto',
                        onPressed: () => provider.setAutoMode(),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: _buildModeButton(
                        context,
                        icon: Icons.touch_app,
                        label: 'Manual',
                        isActive: provider.systemStatus?.mode.name == 'manual',
                        onPressed: () => provider.setManualMode(),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: _buildModeButton(
                        context,
                        icon: Icons.schedule,
                        label: 'Scheduled',
                        isActive: provider.systemStatus?.mode.name == 'scheduled',
                        onPressed: () => provider.setScheduledMode(),
                      ),
                    ),
                  ],
                ),
                
                // Test Pumps Row
                const SizedBox(height: 16),
                Text(
                  'Test Pumps',
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                    fontWeight: FontWeight.w500,
                  ),
                ),
                const SizedBox(height: 8),
                Row(
                  children: [
                    Expanded(
                      child: _buildTestButton(
                        context,
                        label: 'Test Primary',
                        pumpType: PumpType.primary,
                        provider: provider,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: _buildTestButton(
                        context,
                        label: 'Test Secondary',
                        pumpType: PumpType.secondary,
                        provider: provider,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildActionButton(
    BuildContext context, {
    required IconData icon,
    required String label,
    required Color color,
    required VoidCallback? onPressed,
  }) {
    return ElevatedButton.icon(
      onPressed: onPressed,
      icon: Icon(icon, size: 18),
      label: Text(
        label,
        style: const TextStyle(fontSize: 12),
      ),
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        foregroundColor: Colors.white,
        padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 8),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(8),
        ),
      ),
    );
  }

  Widget _buildModeButton(
    BuildContext context, {
    required IconData icon,
    required String label,
    required bool isActive,
    required VoidCallback onPressed,
  }) {
    return OutlinedButton.icon(
      onPressed: isActive ? null : onPressed,
      icon: Icon(icon, size: 16),
      label: Text(
        label,
        style: const TextStyle(fontSize: 10),
      ),
      style: OutlinedButton.styleFrom(
        backgroundColor: isActive ? Colors.blue.withOpacity(0.1) : null,
        foregroundColor: isActive ? Colors.blue : null,
        side: BorderSide(
          color: isActive ? Colors.blue : Colors.grey,
        ),
        padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 4),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(6),
        ),
      ),
    );
  }

  Widget _buildTestButton(
    BuildContext context, {
    required String label,
    required PumpType pumpType,
    required WaterSystemProvider provider,
  }) {
    final pump = pumpType == PumpType.primary 
        ? provider.primaryPump 
        : provider.secondaryPump;
    
    final isRunning = pump?.isRunning ?? false;
    
    return OutlinedButton(
      onPressed: isRunning 
          ? null 
          : () => _showTestPumpDialog(context, provider, pumpType),
      style: OutlinedButton.styleFrom(
        padding: const EdgeInsets.symmetric(vertical: 8),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(6),
        ),
      ),
      child: Text(
        label,
        style: const TextStyle(fontSize: 12),
      ),
    );
  }

  void _showEmergencyStopDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Row(
          children: [
            Icon(Icons.emergency, color: Colors.red),
            SizedBox(width: 8),
            Text('Emergency Stop'),
          ],
        ),
        content: const Text(
          'This will immediately stop all pumps and set the system to emergency mode. '
          'Are you sure you want to proceed?'
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.emergencyStop();
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(
                    content: Text('Emergency stop activated'),
                    backgroundColor: Colors.red,
                  ),
                );
              }
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Emergency Stop'),
          ),
        ],
      ),
    );
  }

  void _showStopPumpDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Stop Pumps'),
        content: const Text('Are you sure you want to stop all pumps?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              final success = await provider.stopPump();
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(success ? 'Pumps stopped' : 'Failed to stop pumps'),
                    backgroundColor: success ? Colors.green : Colors.red,
                  ),
                );
              }
            },
            child: const Text('Stop'),
          ),
        ],
      ),
    );
  }

  void _showStartPumpDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Start Pump'),
        content: const Text('Which pump would you like to start?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              final success = await provider.startPump(PumpType.primary);
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(success ? 'Primary pump started' : 'Failed to start pump'),
                    backgroundColor: success ? Colors.green : Colors.red,
                  ),
                );
              }
            },
            child: const Text('Primary'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              final success = await provider.startPump(PumpType.secondary);
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(success ? 'Secondary pump started' : 'Failed to start pump'),
                    backgroundColor: success ? Colors.green : Colors.red,
                  ),
                );
              }
            },
            child: const Text('Secondary'),
          ),
        ],
      ),
    );
  }

  void _showSwitchPumpDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Switch Pump'),
        content: const Text(
          'This will switch to the backup pump. The current pump will be stopped '
          'and the backup pump will be started.'
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              final success = await provider.switchPump();
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(success ? 'Pump switched' : 'Failed to switch pump'),
                    backgroundColor: success ? Colors.green : Colors.red,
                  ),
                );
              }
            },
            child: const Text('Switch'),
          ),
        ],
      ),
    );
  }

  void _showTestPumpDialog(BuildContext context, WaterSystemProvider provider, PumpType pumpType) {
    final pumpName = pumpType == PumpType.primary ? 'Primary' : 'Secondary';
    
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Test $pumpName Pump'),
        content: Text(
          'This will run the $pumpName pump for a short test cycle. '
          'The pump will automatically stop after the test.'
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              final success = await provider.testPump(pumpType);
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(success 
                        ? '$pumpName pump test started' 
                        : 'Failed to start pump test'),
                    backgroundColor: success ? Colors.green : Colors.red,
                  ),
                );
              }
            },
            child: const Text('Start Test'),
          ),
        ],
      ),
    );
  }
}