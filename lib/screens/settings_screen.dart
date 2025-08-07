import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/water_system_provider.dart';
import '../services/notification_service.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _ipController = TextEditingController();
  final NotificationService _notificationService = NotificationService();

  @override
  void initState() {
    super.initState();
    _ipController.text = '192.168.1.197'; // Default IP
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
      ),
      body: Consumer<WaterSystemProvider>(
        builder: (context, provider, child) {
          return ListView(
            padding: const EdgeInsets.all(16),
            children: [
              // Connection Settings
              _buildSectionCard(
                'Connection Settings',
                [
                  ListTile(
                    leading: const Icon(Icons.wifi),
                    title: const Text('Arduino IP Address'),
                    subtitle: TextField(
                      controller: _ipController,
                      decoration: const InputDecoration(
                        hintText: '192.168.1.197',
                        border: OutlineInputBorder(),
                      ),
                      onSubmitted: (value) {
                        provider.setArduinoIP(value);
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(content: Text('IP address updated')),
                        );
                      },
                    ),
                  ),
                  ListTile(
                    leading: const Icon(Icons.refresh),
                    title: const Text('Connection Status'),
                    subtitle: Text(provider.isConnected ? 'Connected' : 'Disconnected'),
                    trailing: Icon(
                      provider.isConnected ? Icons.check_circle : Icons.error,
                      color: provider.isConnected ? Colors.green : Colors.red,
                    ),
                    onTap: () => provider.refreshData(),
                  ),
                ],
              ),
              const SizedBox(height: 16),

              // Notification Settings
              _buildSectionCard(
                'Notifications',
                [
                  FutureBuilder<bool>(
                    future: _notificationService.areNotificationsEnabled(),
                    builder: (context, snapshot) {
                      return SwitchListTile(
                        secondary: const Icon(Icons.notifications),
                        title: const Text('Enable Notifications'),
                        subtitle: const Text('Receive push notifications for alerts'),
                        value: snapshot.data ?? true,
                        onChanged: (value) {
                          _notificationService.setNotificationsEnabled(value);
                          setState(() {});
                        },
                      );
                    },
                  ),
                  FutureBuilder<bool>(
                    future: _notificationService.areCriticalAlertsEnabled(),
                    builder: (context, snapshot) {
                      return SwitchListTile(
                        secondary: const Icon(Icons.error),
                        title: const Text('Critical Alerts'),
                        subtitle: const Text('Emergency and critical system alerts'),
                        value: snapshot.data ?? true,
                        onChanged: (value) {
                          _notificationService.setCriticalAlertsEnabled(value);
                          setState(() {});
                        },
                      );
                    },
                  ),
                  FutureBuilder<bool>(
                    future: _notificationService.areWarningAlertsEnabled(),
                    builder: (context, snapshot) {
                      return SwitchListTile(
                        secondary: const Icon(Icons.warning),
                        title: const Text('Warning Alerts'),
                        subtitle: const Text('Low levels and maintenance warnings'),
                        value: snapshot.data ?? true,
                        onChanged: (value) {
                          _notificationService.setWarningAlertsEnabled(value);
                          setState(() {});
                        },
                      );
                    },
                  ),
                ],
              ),
              const SizedBox(height: 16),

              // System Settings
              _buildSectionCard(
                'System',
                [
                  ListTile(
                    leading: const Icon(Icons.info),
                    title: const Text('App Version'),
                    subtitle: const Text('1.0.0'),
                  ),
                  ListTile(
                    leading: const Icon(Icons.storage),
                    title: const Text('Clear Data'),
                    subtitle: const Text('Clear all stored data and logs'),
                    onTap: () => _showClearDataDialog(),
                  ),
                  ListTile(
                    leading: const Icon(Icons.restart_alt),
                    title: const Text('Reset System'),
                    subtitle: const Text('Reset Arduino system to defaults'),
                    onTap: () => _showResetSystemDialog(provider),
                  ),
                ],
              ),
              const SizedBox(height: 16),

              // About Section
              _buildSectionCard(
                'About',
                [
                  ListTile(
                    leading: const Icon(Icons.water_drop),
                    title: const Text('Water Level Automation'),
                    subtitle: const Text('Smart water management system'),
                  ),
                  ListTile(
                    leading: const Icon(Icons.developer_mode),
                    title: const Text('Developer'),
                    subtitle: const Text('Built with Flutter & Arduino'),
                  ),
                ],
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildSectionCard(String title, List<Widget> children) {
    return Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Padding(
            padding: const EdgeInsets.all(16),
            child: Text(
              title,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
          ),
          ...children,
        ],
      ),
    );
  }

  void _showClearDataDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Clear Data'),
        content: const Text(
          'This will clear all stored data including logs and settings. '
          'This action cannot be undone. Continue?'
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Implement clear data
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Data cleared')),
              );
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Clear'),
          ),
        ],
      ),
    );
  }

  void _showResetSystemDialog(WaterSystemProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Reset System'),
        content: const Text(
          'This will reset the Arduino system to factory defaults. '
          'All schedules and settings will be lost. Continue?'
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(context);
              // TODO: Implement system reset
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('System reset initiated')),
              );
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Reset'),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _ipController.dispose();
    super.dispose();
  }
}