import * as publicEnv from '$env/static/public';

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

const readPublicEnv = (key: string) => {
	const value = (publicEnv as Record<string, unknown>)[key];
	return typeof value === 'string' ? value : '';
};

// Keep infrastructure values fixed; users only supply credentials.
const HARDCODED_HOST = readPublicEnv('PUBLIC_MQTT_HOST').trim();
const HARDCODED_PORT = readPublicEnv('PUBLIC_MQTT_PORT').trim() || '8884';
const HARDCODED_PATH = (() => {
	const p = readPublicEnv('PUBLIC_MQTT_PATH') || '/mqtt';
	const t = p.trim();
	if (!t) return '/mqtt';
	return t.startsWith('/') ? t : `/${t}`;
})();
const HARDCODED_USE_SSL = readPublicEnv('PUBLIC_MQTT_USE_SSL').toLowerCase() !== 'false';
const HARDCODED_COMMAND_TOPIC = readPublicEnv('PUBLIC_MQTT_COMMAND_TOPIC') || 'water-system/cmd';
const HARDCODED_STATUS_TOPIC = readPublicEnv('PUBLIC_MQTT_STATUS_TOPIC') || 'water-system/status';
const HARDCODED_HEARTBEAT_TOPIC =
	readPublicEnv('PUBLIC_MQTT_HEARTBEAT_TOPIC') || 'water-system/heartbeat';
const HARDCODED_CLIENT_ID_PREFIX = readPublicEnv('PUBLIC_MQTT_CLIENT_ID_PREFIX') || 'water-pwa';

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

function fallbackMotorStatus(activeMotor: ActiveMotor, motor: 'borewell' | 'sump') {
	return activeMotor === motor ? 'running' : 'stopped';
}

// Merge saved credentials with fixed infrastructure settings.
export function createDefaultBrokerSettings(
	overrides: Pick<BrokerSettings, 'username' | 'password'> = { username: '', password: '' }
): BrokerSettings {
	return {
		host: HARDCODED_HOST,
		port: HARDCODED_PORT,
		path: HARDCODED_PATH,
		useSSL: HARDCODED_USE_SSL,
		commandTopic: HARDCODED_COMMAND_TOPIC,
		statusTopic: HARDCODED_STATUS_TOPIC,
		heartbeatTopic: HARDCODED_HEARTBEAT_TOPIC,
		clientIdPrefix: HARDCODED_CLIENT_ID_PREFIX,
		username: overrides.username,
		password: overrides.password
	};
}

// Enforce fixed infra settings during connect.
export function sanitizeBrokerSettings(settings: BrokerSettings): BrokerSettings {
	return {
		...settings,
		host: HARDCODED_HOST,
		port: HARDCODED_PORT,
		path: HARDCODED_PATH,
		useSSL: HARDCODED_USE_SSL,
		commandTopic: HARDCODED_COMMAND_TOPIC,
		statusTopic: HARDCODED_STATUS_TOPIC,
		heartbeatTopic: HARDCODED_HEARTBEAT_TOPIC,
		clientIdPrefix: HARDCODED_CLIENT_ID_PREFIX
	};
}

// Validate required env before allowing connect.
export function validateBrowserBrokerSettings(): string | null {
	if (!HARDCODED_HOST) {
		return 'Broker host is not configured. Set PUBLIC_MQTT_HOST in your .env file.';
	}
	return null;
}

export function buildBrokerUrl() {
	const protocol = HARDCODED_USE_SSL ? 'wss' : 'ws';
	const port = HARDCODED_PORT ? `:${HARDCODED_PORT}` : '';
	return `${protocol}://${HARDCODED_HOST}${port}${HARDCODED_PATH}`;
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

	if (payload.manual_target !== undefined)
		status.manual_target = readEnum(payload.manual_target, activeMotors, 'manual_target');
	if (payload.borewell_status !== undefined)
		status.borewell_status = readEnum(
			payload.borewell_status,
			motorRuntimeStatuses,
			'borewell_status'
		);
	if (payload.sump_status !== undefined)
		status.sump_status = readEnum(payload.sump_status, motorRuntimeStatuses, 'sump_status');
	if (payload.sump_warning !== undefined)
		status.sump_warning = readBoolean(payload.sump_warning, 'sump_warning');
	if (payload.emergency_stop !== undefined)
		status.emergency_stop = readBoolean(payload.emergency_stop, 'emergency_stop');
	if (payload.auto_prefer_sump !== undefined)
		status.auto_prefer_sump = readBoolean(payload.auto_prefer_sump, 'auto_prefer_sump');
	if (payload.wifi_status !== undefined)
		status.wifi_status = readEnum(payload.wifi_status, wifiConnectionPhases, 'wifi_status');
	if (payload.mqtt_connected !== undefined)
		status.mqtt_status = readBoolean(payload.mqtt_connected, 'mqtt_connected')
			? 'connected'
			: 'disconnected';

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
			borewell: { active: parsed.motor === 'borewell', status: borewellStatus },
			sump: { active: parsed.motor === 'sump', status: sumpTransferStatus }
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
	sump: 'Sump'
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

// QR payload contains credentials only.
export function encodeBrokerSettingsAsQR(settings: BrokerSettings): string {
	const params = new URLSearchParams({
		username: settings.username,
		password: settings.password
	});
	return params.toString();
}

// Decode QR payload produced by encodeBrokerSettingsAsQR.
export function decodeBrokerSettingsFromQR(raw: string): Partial<BrokerSettings> | null {
	try {
		// Accept both query strings and full URLs.
		const queryStart = raw.indexOf('?');
		const queryString = queryStart === -1 ? raw : raw.slice(queryStart + 1);
		const params = new URLSearchParams(queryString);

		const username = params.get('username');
		const password = params.get('password');

		// Reject unrelated QR payloads.
		if (username === null && password === null) return null;

		const result: Partial<BrokerSettings> = {};
		if (username !== null) result.username = username;
		if (password !== null) result.password = password;
		return result;
	} catch {
		return null;
	}
}
