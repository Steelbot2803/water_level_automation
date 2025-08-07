import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/water_system_provider.dart';
import '../models/alert.dart';
import '../widgets/tank_level_widget.dart';
import '../widgets/pump_status_widget.dart';
import '../widgets/system_status_widget.dart';
import '../widgets/quick_actions_widget.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  _DashboardScreenState createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  @override
  void initState() {
    super.initState();
    // Refresh data when screen loads
    WidgetsBinding.instance.addPostFrameCallback((_) {
      Provider.of<WaterSystemProvider>(context, listen: false).refreshData();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Water Level Dashboard'),
        actions: [
          Consumer<WaterSystemProvider>(
            builder: (context, provider, child) {
              return IconButton(
                icon: Icon(
                  provider.isConnected ? Icons.wifi : Icons.wifi_off,
                  color: provider.isConnected ? Colors.green : Colors.red,
                ),
                onPressed: () => provider.refreshData(),
              );
            },
          ),
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
          if (provider.isLoading && provider.tankLevels.isEmpty) {
            return const Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircularProgressIndicator(),
                  SizedBox(height: 16),
                  Text('Loading system data...'),
                ],
              ),
            );
          }

          if (provider.errorMessage != null && provider.tankLevels.isEmpty) {
            return Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(
                    Icons.error_outline,
                    size: 64,
                    color: Colors.red,
                  ),
                  const SizedBox(height: 16),
                  Text(
                    'Connection Error',
                    style: Theme.of(context).textTheme.headlineSmall,
                  ),
                  const SizedBox(height: 8),
                  Text(
                    provider.errorMessage!,
                    textAlign: TextAlign.center,
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                  const SizedBox(height: 16),
                  ElevatedButton(
                    onPressed: () => provider.refreshData(),
                    child: const Text('Retry'),
                  ),
                ],
              ),
            );
          }

          return RefreshIndicator(
            onRefresh: () => provider.refreshData(),
            child: SingleChildScrollView(
              physics: const AlwaysScrollableScrollPhysics(),
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // Critical Alerts Banner
                  if (provider.hasCriticalAlerts) ...[
                    _buildCriticalAlertsBanner(provider),
                    const SizedBox(height: 16),
                  ],

                  // System Status Overview
                  SystemStatusWidget(),
                  const SizedBox(height: 16),

                  // Tank Levels Section
                  _buildSectionHeader('Tank Levels'),
                  const SizedBox(height: 12),
                  Row(
                    children: [
                      Expanded(
                        child: TankLevelWidget(
                          tank: provider.overheadTank!,
                          title: 'Overhead Tank',
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: TankLevelWidget(
                          tank: provider.secondaryTank!,
                          title: 'Secondary Tank',
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),

                  // Pump Status Section
                  _buildSectionHeader('Pump Status'),
                  const SizedBox(height: 12),
                  Row(
                    children: [
                      Expanded(
                        child: PumpStatusWidget(
                          pump: provider.primaryPump!,
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: PumpStatusWidget(
                          pump: provider.secondaryPump!,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),

                  // Quick Actions
                  _buildSectionHeader('Quick Actions'),
                  const SizedBox(height: 12),
                  QuickActionsWidget(),
                  const SizedBox(height: 24),

                  // Recent Alerts
                  if (provider.alerts.isNotEmpty) ...[
                    _buildSectionHeader('Recent Alerts'),
                    const SizedBox(height: 12),
                    _buildRecentAlerts(provider),
                  ],

                  // Last Update Info
                  if (provider.lastUpdate != null) ...[
                    const SizedBox(height: 24),
                    _buildLastUpdateInfo(provider),
                  ],
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  Widget _buildSectionHeader(String title) {
    return Text(
      title,
      style: Theme.of(context).textTheme.titleLarge?.copyWith(
        fontWeight: FontWeight.bold,
      ),
    );
  }

  Widget _buildCriticalAlertsBanner(WaterSystemProvider provider) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.red.withOpacity(0.1),
        border: Border.all(color: Colors.red),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        children: [
          const Icon(Icons.warning, color: Colors.red, size: 24),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Critical Alerts (${provider.criticalAlerts.length})',
                  style: const TextStyle(
                    color: Colors.red,
                    fontWeight: FontWeight.bold,
                    fontSize: 16,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  provider.criticalAlerts.first.message,
                  style: const TextStyle(color: Colors.red),
                ),
              ],
            ),
          ),
          TextButton(
            onPressed: () {
              // Navigate to alerts screen
            },
            child: const Text('View All'),
          ),
        ],
      ),
    );
  }

  Widget _buildRecentAlerts(WaterSystemProvider provider) {
    final recentAlerts = provider.alerts.take(3).toList();
    
    return Card(
      child: Column(
        children: recentAlerts.map((alert) {
          return ListTile(
            leading: Icon(
              _getAlertIcon(alert.type),
              color: _getAlertColor(alert.type),
            ),
            title: Text(alert.title),
            subtitle: Text(alert.message),
            trailing: Text(
              alert.formattedTimestamp,
              style: Theme.of(context).textTheme.bodySmall,
            ),
            onTap: () {
              // Show alert details
              _showAlertDetails(alert);
            },
          );
        }).toList(),
      ),
    );
  }

  Widget _buildLastUpdateInfo(WaterSystemProvider provider) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).cardColor,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.access_time,
            size: 16,
            color: Colors.grey,
          ),
          const SizedBox(width: 8),
          Text(
            'Last updated: ${_formatLastUpdate(provider.lastUpdate!)}',
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: Colors.grey,
            ),
          ),
        ],
      ),
    );
  }

  IconData _getAlertIcon(AlertType type) {
    switch (type) {
      case AlertType.critical:
        return Icons.error;
      case AlertType.warning:
        return Icons.warning;
      case AlertType.info:
        return Icons.info;
    }
  }

  Color _getAlertColor(AlertType type) {
    switch (type) {
      case AlertType.critical:
        return Colors.red;
      case AlertType.warning:
        return Colors.orange;
      case AlertType.info:
        return Colors.blue;
    }
  }

  String _formatLastUpdate(DateTime lastUpdate) {
    final now = DateTime.now();
    final difference = now.difference(lastUpdate);
    
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

  void _showAlertDetails(Alert alert) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(alert.title),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(alert.message),
            const SizedBox(height: 16),
            Row(
              children: [
                Icon(
                  _getAlertIcon(alert.type),
                  color: _getAlertColor(alert.type),
                  size: 16,
                ),
                const SizedBox(width: 8),
                Text(
                  alert.typeDescription,
                  style: TextStyle(
                    color: _getAlertColor(alert.type),
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              'Time: ${alert.timestamp.toString()}',
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          if (!alert.acknowledged)
            ElevatedButton(
              onPressed: () {
                Provider.of<WaterSystemProvider>(context, listen: false)
                    .acknowledgeAlert(alert.id);
                Navigator.pop(context);
              },
              child: const Text('Acknowledge'),
            ),
        ],
      ),
    );
  }
}