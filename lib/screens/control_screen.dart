import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/water_system_provider.dart';
import '../models/pump_status.dart';
import '../widgets/pump_status_widget.dart';

class ControlScreen extends StatelessWidget {
  const ControlScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Pump Control'),
        actions: [
          Consumer<WaterSystemProvider>(
            builder: (context, provider, child) {
              return IconButton(
                icon: provider.isLoading 
                    ? const SizedBox(
                        width: 20,
                        height: 20,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.refresh),
                onPressed: provider.isLoading ? null : () => provider.refreshData(),
              );
            },
          ),
        ],
      ),
      body: Consumer<WaterSystemProvider>(
        builder: (context, provider, child) {
          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // System Mode Control
                _buildSystemModeCard(context, provider),
                const SizedBox(height: 16),
                
                // Pump Status Cards
                Text(
                  'Pump Status',
                  style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 12),
                
                PumpStatusWidget(pump: provider.primaryPump!),
                const SizedBox(height: 16),
                PumpStatusWidget(pump: provider.secondaryPump!),
                const SizedBox(height: 24),
                
                // Manual Control Section
                _buildManualControlCard(context, provider),
                const SizedBox(height: 16),
                
                // Emergency Controls
                _buildEmergencyControlCard(context, provider),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildSystemModeCard(BuildContext context, WaterSystemProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'System Mode',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            
            Row(
              children: [
                Expanded(
                  child: _buildModeButton(
                    context,
                    'Automatic',
                    Icons.auto_mode,
                    provider.systemStatus?.mode.name == 'auto',
                    () => provider.setAutoMode(),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _buildModeButton(
                    context,
                    'Manual',
                    Icons.touch_app,
                    provider.systemStatus?.mode.name == 'manual',
                    () => provider.setManualMode(),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _buildModeButton(
                    context,
                    'Scheduled',
                    Icons.schedule,
                    provider.systemStatus?.mode.name == 'scheduled',
                    () => provider.setScheduledMode(),
                  ),
                ),
              ],
            ),
            
            if (provider.systemStatus != null) ...[
              const SizedBox(height: 12),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Theme.of(context).cardColor.withOpacity(0.5),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  children: [
                    Icon(
                      Icons.info_outline,
                      size: 16,
                      color: Colors.blue,
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'Current mode: ${provider.systemStatus!.modeDescription}',
                        style: Theme.of(context).textTheme.bodySmall,
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
  }

  Widget _buildManualControlCard(BuildContext context, WaterSystemProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Manual Control',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            
            // Start/Stop Controls
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: provider.hasActivePump 
                        ? () => _showStopDialog(context, provider)
                        : () => _showStartDialog(context, provider),
                    icon: Icon(provider.hasActivePump ? Icons.stop : Icons.play_arrow),
                    label: Text(provider.hasActivePump ? 'Stop All Pumps' : 'Start Pump'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: provider.hasActivePump ? Colors.red : Colors.green,
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            
            // Individual Pump Controls
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => _showStartSpecificDialog(context, provider, PumpType.primary),
                    icon: const Icon(Icons.water_drop),
                    label: const Text('Start Primary'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => _showStartSpecificDialog(context, provider, PumpType.secondary),
                    icon: const Icon(Icons.water_drop_outlined),
                    label: const Text('Start Secondary'),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            
            // Switch and Test Controls
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: provider.hasActivePump 
                        ? () => _showSwitchDialog(context, provider)
                        : null,
                    icon: const Icon(Icons.swap_horiz),
                    label: const Text('Switch Pump'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => _showTestDialog(context, provider),
                    icon: const Icon(Icons.play_circle_outline),
                    label: const Text('Test Pumps'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildEmergencyControlCard(BuildContext context, WaterSystemProvider provider) {
    return Card(
      color: Colors.red.withOpacity(0.1),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.emergency, color: Colors.red),
                const SizedBox(width: 8),
                Text(
                  'Emergency Controls',
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                    color: Colors.red,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            
            SizedBox(
              width: double.infinity,
              child: ElevatedButton.icon(
                onPressed: () => _showEmergencyStopDialog(context, provider),
                icon: const Icon(Icons.emergency),
                label: const Text('EMERGENCY STOP'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.red,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(vertical: 16),
                ),
              ),
            ),
            const SizedBox(height: 8),
            
            Text(
              'Use only in emergency situations. This will immediately stop all pumps and disable automatic operation.',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: Colors.red.shade700,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildModeButton(
    BuildContext context,
    String label,
    IconData icon,
    bool isActive,
    VoidCallback onPressed,
  ) {
    return ElevatedButton.icon(
      onPressed: isActive ? null : onPressed,
      icon: Icon(icon, size: 18),
      label: Text(label),
      style: ElevatedButton.styleFrom(
        backgroundColor: isActive ? Colors.blue : null,
        foregroundColor: isActive ? Colors.white : null,
        padding: const EdgeInsets.symmetric(vertical: 12),
      ),
    );
  }

  void _showStopDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Stop All Pumps'),
        content: const Text('Are you sure you want to stop all pumps?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.stopPump();
            },
            child: const Text('Stop'),
          ),
        ],
      ),
    );
  }

  void _showStartDialog(BuildContext context, WaterSystemProvider provider) {
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
              await provider.startPump(PumpType.primary);
            },
            child: const Text('Primary'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.startPump(PumpType.secondary);
            },
            child: const Text('Secondary'),
          ),
        ],
      ),
    );
  }

  void _showStartSpecificDialog(BuildContext context, WaterSystemProvider provider, PumpType pumpType) {
    final pumpName = pumpType == PumpType.primary ? 'Primary' : 'Secondary';
    
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Start $pumpName Pump'),
        content: Text('Are you sure you want to start the $pumpName pump?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.startPump(pumpType);
            },
            child: const Text('Start'),
          ),
        ],
      ),
    );
  }

  void _showSwitchDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Switch Pump'),
        content: const Text('This will switch to the backup pump. Continue?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.switchPump();
            },
            child: const Text('Switch'),
          ),
        ],
      ),
    );
  }

  void _showTestDialog(BuildContext context, WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Test Pumps'),
        content: const Text('Which pump would you like to test?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.testPump(PumpType.primary);
            },
            child: const Text('Test Primary'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              await provider.testPump(PumpType.secondary);
            },
            child: const Text('Test Secondary'),
          ),
        ],
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
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Emergency Stop'),
          ),
        ],
      ),
    );
  }
}