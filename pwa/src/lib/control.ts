import {
	PUBLIC_MQTT_CLIENT_ID_PREFIX,
	PUBLIC_MQTT_COMMAND_TOPIC,
	PUBLIC_MQTT_HOST,
	PUBLIC_MQTT_PASSWORD,
	PUBLIC_MQTT_PATH,
	PUBLIC_MQTT_PORT,
	PUBLIC_MQTT_STATUS_TOPIC,
	PUBLIC_MQTT_USERNAME,
	PUBLIC_MQTT_USE_SSL
} from '$env/static/public';

import {
	activeMotors,
	type ActiveMotor,
	type ArduinoCommand,
	type ArduinoStatusPayload,
	type BrokerSettings,
	type ControlMode,
	type DeviceTelemetry,
	type MotorRuntimeStatus,
	motorRuntimeStatuses,
	mqttModes,
	overheadLevels,
	sumpLevels,
	wifiConnectionPhases
} from './types.js';

const refillLevels = new Set(['critical', 'low']);
const defaultWebSocketPorts = {
	secure: '8884',
	insecure: '8000'
} as const;

function isOneOf<T extends string>(value: unknown, values: readonly T[]): value is T {
	return typeof value === 'string' && (values as readonly string[]).includes(value);
}

function readEnum<T extends string>(value: unknown, values: readonly T[], field: string): T {
	if (!isOneOf(value, values)) {
		throw new Error(`Invalid "${field}" value in MQTT status payload`);
	}

	return value;
}

function readBoolean(value: unknown, field: string): boolean {
	if (typeof value !== 'boolean') {
		throw new Error(`Invalid "${field}" value in MQTT status payload`);
	}

	return value;
}

function normalizePath(path: string) {
	const trimmed = path.trim();
	if (!trimmed) return '/mqtt';
	return trimmed.startsWith('/') ? trimmed : `/${trimmed}`;
}

function fallbackMotorStatus(activeMotor: ActiveMotor, motor: 'borewell' | 'sump') {
	return activeMotor === motor ? 'running' : 'stopped';
}

function defaultPort(useSSL: boolean) {
	return useSSL ? defaultWebSocketPorts.secure : defaultWebSocketPorts.insecure;
}

export function createDefaultBrokerSettings(): BrokerSettings {
	const useSSL = PUBLIC_MQTT_USE_SSL?.toLowerCase() !== 'false';

	return {
		host: PUBLIC_MQTT_HOST?.trim() ?? '',
		port: PUBLIC_MQTT_PORT?.trim() || defaultPort(useSSL),
		path: normalizePath(PUBLIC_MQTT_PATH ?? '/mqtt'),
		username: PUBLIC_MQTT_USERNAME ?? '',
		password: PUBLIC_MQTT_PASSWORD ?? '',
		useSSL,
		commandTopic: PUBLIC_MQTT_COMMAND_TOPIC ?? 'water-system/cmd',
		statusTopic: PUBLIC_MQTT_STATUS_TOPIC ?? 'water-system/status',
		clientIdPrefix: PUBLIC_MQTT_CLIENT_ID_PREFIX ?? 'water-pwa'
	};
}

export function sanitizeBrokerSettings(settings: BrokerSettings): BrokerSettings {
	return {
		...settings,
		host: settings.host.trim(),
		port: settings.port.trim(),
		path: normalizePath(settings.path)
	};
}

export function validateBrowserBrokerSettings(settings: BrokerSettings): string | null {
	const normalized = sanitizeBrokerSettings(settings);

	if (!normalized.host) {
		return 'Broker host is required.';
	}

	if (normalized.useSSL && normalized.port === '8883') {
		return 'Use HiveMQ secure WebSocket port `8884` in the PWA. Port `8883` is the Arduino MQTT/TLS port.';
	}

	if (!normalized.useSSL && normalized.port === '1883') {
		return 'Use a WebSocket listener for the PWA. Port `1883` is raw MQTT, not browser WebSockets.';
	}

	return null;
}

export function buildBrokerUrl(settings: BrokerSettings) {
	const normalized = sanitizeBrokerSettings(settings);
	const protocol = normalized.useSSL ? 'wss' : 'ws';
	const port = normalized.port ? `:${normalized.port}` : '';
	return `${protocol}://${normalized.host}${port}${normalized.path}`;
}

export function parseArduinoStatusPayload(rawPayload: string): ArduinoStatusPayload {
	let parsed: unknown;

	try {
		parsed = JSON.parse(rawPayload);
	} catch {
		throw new Error('MQTT status payload is not valid JSON');
	}

	if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
		throw new Error('MQTT status payload must be a JSON object');
	}

	const payload = parsed as Record<string, unknown>;
	const status: ArduinoStatusPayload = {
		mode: readEnum(payload.mode, mqttModes, 'mode'),
		override: readBoolean(payload.override, 'override'),
		overhead: readEnum(payload.overhead, overheadLevels, 'overhead'),
		sump: readEnum(payload.sump, sumpLevels, 'sump'),
		motor: readEnum(payload.motor, activeMotors, 'motor')
	};

	if (payload.manual_target !== undefined) {
		status.manual_target = readEnum(payload.manual_target, activeMotors, 'manual_target');
	}

	if (payload.borewell_status !== undefined) {
		status.borewell_status = readEnum(
			payload.borewell_status,
			motorRuntimeStatuses,
			'borewell_status'
		);
	}

	if (payload.sump_status !== undefined) {
		status.sump_status = readEnum(payload.sump_status, motorRuntimeStatuses, 'sump_status');
	}

	if (payload.sump_warning !== undefined) {
		status.sump_warning = readBoolean(payload.sump_warning, 'sump_warning');
	}

	if (payload.emergency_stop !== undefined) {
		status.emergency_stop = readBoolean(payload.emergency_stop, 'emergency_stop');
	}

	if (payload.auto_prefer_sump !== undefined) {
		status.auto_prefer_sump = readBoolean(payload.auto_prefer_sump, 'auto_prefer_sump');
	}
	if (payload.wifi_status !== undefined) {
		status.wifi_status = readEnum(
			payload.wifi_status,
			wifiConnectionPhases, // you need to add this constant — see below
			'wifi_status'
		);
	}
	if (payload.mqtt_connected !== undefined) {
		status.mqtt_status = readBoolean(payload.mqtt_connected, 'mqtt_connected')
			? 'connected'
			: 'disconnected';
	}

	return status;
}

export function deriveControlMode(payload: ArduinoStatusPayload): ControlMode {
	if (payload.mode === 'manual') return 'manual';
	return payload.override ? 'override_fill' : 'auto';
}

export function toDeviceTelemetry(rawPayload: string, receivedAt = Date.now()): DeviceTelemetry {
	const parsed = parseArduinoStatusPayload(rawPayload);
	const controlMode = deriveControlMode(parsed);
	const manualTarget = parsed.manual_target ?? (parsed.mode === 'manual' ? parsed.motor : 'none');
	const borewellStatus = parsed.borewell_status ?? fallbackMotorStatus(parsed.motor, 'borewell');
	const sumpTransferStatus = parsed.sump_status ?? fallbackMotorStatus(parsed.motor, 'sump');
	const sumpWarning = parsed.sump_warning ?? parsed.sump === 'critical';

	return {
		...parsed,
		manual_target: manualTarget,
		borewell_status: borewellStatus,
		sump_status: sumpTransferStatus,
		sump_warning: sumpWarning,
		controlMode,
		needFill: parsed.override || refillLevels.has(parsed.overhead),
		receivedAt,
		rawPayload,
		alarms: {
			overheadCritical: parsed.overhead === 'critical',
			sumpCritical: parsed.sump === 'critical',
			emergencyStop: parsed.emergency_stop ?? false
		},
		motors: {
			borewell: {
				active: parsed.motor === 'borewell',
				status: borewellStatus
			},
			sump: {
				active: parsed.motor === 'sump',
				status: sumpTransferStatus
			}
		}
	};
}

export function createCommandLogId(command: ArduinoCommand, at: number) {
	return `${at}-${command.replace(/\s+/g, '-')}`;
}

export function createClientId(prefix: string) {
	if (typeof crypto !== 'undefined' && typeof crypto.randomUUID === 'function') {
		return `${prefix}-${crypto.randomUUID().slice(0, 8)}`;
	}

	return `${prefix}-${Date.now().toString(36)}`;
}

export const controlModeLabels: Record<ControlMode, string> = {
	auto: 'Automatic',
	override_fill: 'Override Fill',
	manual: 'Manual'
};

export const motorLabels: Record<ActiveMotor, string> = {
	none: 'None',
	borewell: 'Borewell',
	sump: 'Sump Transfer'
};

export const runtimeStatusLabels: Record<MotorRuntimeStatus, string> = {
	stopped: 'Stopped',
	starting: 'Starting',
	running: 'Running',
	dry_run_lock: 'Dry Run Lock',
	sump_critical: 'Sump Level Critical'
};

export const commandLabels: Record<ArduinoCommand, string> = {
	auto: 'Auto Mode',
	manual: 'Manual Mode',
	override: 'Override Fill',
	'motor borewell': 'Run Borewell',
	'motor sump': 'Run Sump',
	'motor stop': 'Stop Motor',
	borewell: 'Prefer Borewell',
	sump: 'Prefer Sump',
	'unlock borewell': 'Unlock Borewell',
	'unlock sump': 'Unlock Sump',
	estop: 'E–Stop',
	resume: 'Resume',
	reset: 'Reset System',
	'reset state': 'Reset State'
};
