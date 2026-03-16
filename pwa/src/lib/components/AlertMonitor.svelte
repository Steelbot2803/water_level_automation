<script lang="ts">
	import { onMount } from 'svelte';

	import { alerts } from '$lib/stores/alerts.js';
	import { waterSystem } from '$lib/stores/system.js';
	import type { CommandLogEntry, DeviceTelemetry, WaterAutomationState } from '$lib/types.js';

	function notifyConnectionChanges(previous: WaterAutomationState | null, next: WaterAutomationState) {
		if (!previous) {
			if (next.connection.phase === 'error' && next.connection.lastError) {
				alerts.push({
					title: 'Connection error',
					message: next.connection.lastError,
					severity: 'error',
					tag: 'connection-error-initial',
					cooldownMs: 15000
				});
			}
			return;
		}

		if (next.connection.phase === 'reconnecting' && previous.connection.phase !== 'reconnecting') {
			alerts.push({
				title: 'Connection lost',
				message: 'Broker connection dropped. Retrying automatically.',
				severity: 'warning',
				tag: 'connection-reconnecting',
				cooldownMs: 15000
			});
		}

		if (
			next.connection.phase === 'connected' &&
			(previous.connection.phase === 'reconnecting' ||
				previous.connection.phase === 'offline' ||
				previous.connection.phase === 'error')
		) {
			alerts.push({
				title: 'Connection restored',
				message: 'Broker connection is back and telemetry is live again.',
				severity: 'success',
				tag: 'connection-restored',
				cooldownMs: 10000
			});
		}

		if (
			next.connection.phase === 'error' &&
			(previous.connection.phase !== 'error' ||
				previous.connection.lastError !== next.connection.lastError)
		) {
			alerts.push({
				title: 'Connection error',
				message: next.connection.lastError ?? next.connection.detail,
				severity: 'error',
				tag: 'connection-error',
				cooldownMs: 15000
			});
		}
	}

	function notifyCommandFailures(previous: CommandLogEntry[], next: CommandLogEntry[]) {
		if (!next.length) return;

		const previousIds = new Set(previous.map((entry) => entry.id));
		const newEntries = next.filter((entry) => !previousIds.has(entry.id));

		for (const entry of newEntries) {
			if (entry.status !== 'failed') continue;

			alerts.push({
				title: 'Command failed',
				message: entry.error ? `${entry.command}: ${entry.error}` : `${entry.command} failed to send.`,
				severity: 'error',
				tag: `command-failed-${entry.command}`,
				cooldownMs: 5000
			});
		}
	}

	function notifyTelemetryChanges(previous: DeviceTelemetry | null, next: DeviceTelemetry | null) {
		if (!next) return;

		if (!previous || previous.alarms.overheadCritical !== next.alarms.overheadCritical) {
			if (next.alarms.overheadCritical) {
				alerts.push({
					title: 'Overhead tank critical',
					message: `Overhead tank is ${next.overhead}.`,
					severity: 'error',
					tag: 'alarm-overhead-critical',
					cooldownMs: 120000
				});
			} else if (previous) {
				alerts.push({
					title: 'Overhead tank recovered',
					message: `Overhead tank is back to ${next.overhead}.`,
					severity: 'success',
					tag: 'alarm-overhead-recovered',
					cooldownMs: 30000
				});
			}
		}

		if (!previous || previous.alarms.sumpCritical !== next.alarms.sumpCritical) {
			if (next.alarms.sumpCritical) {
				alerts.push({
					title: 'Sump tank critical',
					message: `Sump tank is ${next.sump}.`,
					severity: 'error',
					tag: 'alarm-sump-critical',
					cooldownMs: 120000
				});
			} else if (previous) {
				alerts.push({
					title: 'Sump tank recovered',
					message: `Sump tank is back to ${next.sump}.`,
					severity: 'success',
					tag: 'alarm-sump-critical-recovered',
					cooldownMs: 30000
				});
			}
		}

		notifyMotorStatusChange(
			'Borewell motor',
			previous?.motors.borewell.status ?? null,
			next.motors.borewell.status
		);
		notifyMotorStatusChange(
			'Sump transfer motor',
			previous?.motors.sump.status ?? null,
			next.motors.sump.status
		);
	}

	function notifyMotorStatusChange(
		label: string,
		previousStatus: DeviceTelemetry['motors']['borewell']['status'] | null,
		nextStatus: DeviceTelemetry['motors']['borewell']['status']
	) {
		if (previousStatus === nextStatus) return;

		if (nextStatus === 'dry_run_lock') {
			alerts.push({
				title: `${label} dry-run lock`,
				message: `${label} stopped after dry-run protection triggered.`,
				severity: 'error',
				tag: `${label}-dry-run-lock`,
				cooldownMs: 120000
			});
			return;
		}
	}

	onMount(() => {
		const cleanupAlerts = alerts.initialize();
		let previousState: WaterAutomationState | null = null;

		const unsubscribe = waterSystem.subscribe((state) => {
			notifyConnectionChanges(previousState, state);
			notifyCommandFailures(previousState?.recentCommands ?? [], state.recentCommands);
			notifyTelemetryChanges(previousState?.device ?? null, state.device);

			previousState = state;
		});

		return () => {
			unsubscribe();
			cleanupAlerts();
		};
	});
</script>
