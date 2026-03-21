<svelte:options runes={true} />

<script lang="ts">
	import { onMount } from 'svelte';
	import { get } from 'svelte/store';
	import { alerts } from '$lib/stores/alerts.js';
	import { notificationPrefs } from '$lib/stores/notifications.js';
	import { waterSystem } from '$lib/stores/system.js';
	import type {
		CommandLogEntry,
		DeviceTelemetry,
		MotorRuntimeStatus,
		WaterAutomationState
	} from '$lib/types.js';

	function notifyConnectionChanges(
		previous: WaterAutomationState | null,
		next: WaterAutomationState
	) {
		if (!previous) {
			if (next.mqttConnection.mqttPhase === 'error' && next.mqttConnection.lastError) {
				alerts.push({
					title: 'Connection error',
					message: next.mqttConnection.lastError,
					severity: 'error',
					tag: 'mqttConnection-error-initial',
					cooldownMs: 15000
				});
			}
			return;
		}

		if (
			next.mqttConnection.mqttPhase === 'reconnecting' &&
			previous.mqttConnection.mqttPhase !== 'reconnecting'
		) {
			if (get(notificationPrefs).connectionLost)
				alerts.push({
					title: 'Connection lost',
					message: 'Broker connection dropped. Retrying automatically.',
					severity: 'warning',
					tag: 'mqttConnection-reconnecting',
					cooldownMs: 15000
				});
		}

		if (
			next.mqttConnection.mqttPhase === 'connected' &&
			(previous.mqttConnection.mqttPhase === 'reconnecting' ||
				previous.mqttConnection.mqttPhase === 'offline' ||
				previous.mqttConnection.mqttPhase === 'error')
		) {
			alerts.push({
				title: 'Connection restored',
				message: 'Broker connection is back and telemetry is live again.',
				severity: 'success',
				tag: 'mqttConnection-restored',
				cooldownMs: 10000
			});
		}

		if (
			next.mqttConnection.mqttPhase === 'error' &&
			(previous.mqttConnection.mqttPhase !== 'error' ||
				previous.mqttConnection.lastError !== next.mqttConnection.lastError)
		) {
			alerts.push({
				title: 'Connection error',
				message: next.mqttConnection.lastError ?? next.mqttConnection.detail,
				severity: 'error',
				tag: 'mqttConnection-error',
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
				message: entry.error
					? `${entry.command}: ${entry.error}`
					: `${entry.command} failed to send.`,
				severity: 'error',
				tag: `command-failed-${entry.command}`,
				cooldownMs: 5000
			});
		}
	}

	function notifyTelemetryChanges(previous: DeviceTelemetry | null, next: DeviceTelemetry | null) {
		if (!next) return;

		// Emergency stop activated
		// Emergency stop activated
		if (next.alarms.emergencyStop && !previous?.alarms.emergencyStop) {
			if (get(notificationPrefs).emergencyStop)
				alerts.push({
					title: 'Emergency stop active',
					message: 'All motors are halted until the stop is cleared.',
					severity: 'error',
					tag: 'alarm-emergency-stop',
					cooldownMs: 30000
				});
		}

		// Overhead tank critical
		if (!previous || previous.alarms.overheadCritical !== next.alarms.overheadCritical) {
			if (next.alarms.overheadCritical) {
				if (get(notificationPrefs).overheadCritical)
					alerts.push({
						title: 'Overhead tank critical',
						message: `Overhead tank is ${next.overhead}. Filling started.`,
						severity: 'error',
						tag: 'alarm-overhead-critical',
						cooldownMs: 120000
					});
			} else if (previous) {
				if (get(notificationPrefs).overheadCritical)
					alerts.push({
						title: 'Overhead tank recovered',
						message: `Overhead tank is back to ${next.overhead}.`,
						severity: 'success',
						tag: 'alarm-overhead-recovered',
						cooldownMs: 30000
					});
			}
		}

		// Overhead dropped to low (filling kicks in automatically)
		if (
			previous &&
			previous.overhead !== next.overhead &&
			next.overhead === 'low' &&
			previous.overhead !== 'critical'
		) {
			if (get(notificationPrefs).overheadLow)
				alerts.push({
					title: 'Overhead tank low',
					message: 'Overhead is LOW. Auto-fill started.',
					severity: 'warning',
					tag: 'alarm-overhead-low',
					cooldownMs: 120000
				});
		}

		// Overhead reached high (fill complete)
		if (previous && previous.overhead !== 'high' && next.overhead === 'high') {
			if (get(notificationPrefs).overheadFull)
				alerts.push({
					title: 'Overhead tank full',
					message: 'Overhead tank reached HIGH. Pumping stopped.',
					severity: 'success',
					tag: 'alarm-overhead-high',
					cooldownMs: 60000
				});
		}

		// Sump tank critical
		if (!previous || previous.alarms.sumpCritical !== next.alarms.sumpCritical) {
			if (next.alarms.sumpCritical) {
				if (get(notificationPrefs).sumpCritical)
					alerts.push({
						title: 'Sump tank critical',
						message: 'Sump tank is CRITICAL. Sump transfer motor is blocked.',
						severity: 'error',
						tag: 'alarm-sump-critical',
						cooldownMs: 120000
					});
			} else if (previous) {
				if (get(notificationPrefs).sumpCritical)
					alerts.push({
						title: 'Sump tank recovered',
						message: `Sump tank is back to ${next.sump}.`,
						severity: 'success',
						tag: 'alarm-sump-critical-recovered',
						cooldownMs: 30000
					});
			}
		}

		// Motor status changes
		notifyMotorStatusChange(
			'Borewell motor',
			'borewell',
			previous?.motors.borewell.status ?? null,
			next.motors.borewell.status,
			next
		);
		notifyMotorStatusChange(
			'Sump transfer motor',
			'sump',
			previous?.motors.sump.status ?? null,
			next.motors.sump.status,
			next
		);
	}

	function notifyMotorStatusChange(
		label: string,
		motorKey: 'borewell' | 'sump',
		previousStatus: MotorRuntimeStatus | null,
		nextStatus: MotorRuntimeStatus,
		telemetry: DeviceTelemetry
	) {
		if (previousStatus === nextStatus) return;

		if (nextStatus === 'dry_run_lock') {
			if (motorKey === 'borewell') {
				if (get(notificationPrefs).borewellDryRun) {
					const sumpAvailable = telemetry.sump !== 'critical';
					alerts.push({
						title: 'Borewell motor: dry-run protection',
						message: sumpAvailable
							? 'Borewell stopped after no-flow detection. Sump transfer remains available.'
							: 'Borewell stopped after no-flow detection. No alternate motor is available.',
						severity: 'error',
						tag: 'borewell-dry-run-lock',
						cooldownMs: 120000
					});
				}
			} else {
				if (get(notificationPrefs).sumpDryRun)
					alerts.push({
						title: 'Sump transfer motor: dry-run protection',
						message: 'Sump transfer stopped after no-flow detection.',
						severity: 'error',
						tag: 'sump-dry-run-lock',
						cooldownMs: 120000
					});
			}
			return;
		}

		if (nextStatus === 'sump_critical') {
			if (get(notificationPrefs).sumpCritical)
				alerts.push({
					title: 'Sump transfer motor blocked',
					message: 'Sump transfer is blocked because the sump tank is critical.',
					severity: 'warning',
					tag: 'sump-motor-blocked-critical',
					cooldownMs: 120000
				});
			return;
		}
	}

	onMount(() => {
		const cleanupAlerts = alerts.initialize();
		notificationPrefs.initialize();
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
