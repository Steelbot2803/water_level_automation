import { browser } from '$app/environment';
import mqtt, { type MqttClient } from 'mqtt';
import { derived, get, writable } from 'svelte/store';
import { perf } from '../perf.js';

import {
	buildBrokerUrl,
	createClientId,
	createCommandLogId,
	createDefaultBrokerSettings,
	sanitizeBrokerSettings,
	toDeviceTelemetry,
	validateBrowserBrokerSettings
} from '../control.js';
import type {
	ArduinoCommand,
	BrokerSettings,
	CommandLogEntry,
	WaterAutomationState
} from '../types.js';

const COMMAND_HISTORY_LIMIT = 8;
const CONNECTION_TIMEOUT_MS = 10000;
const MAX_RECONNECT_ATTEMPTS = 3;
const RECONNECT_PERIOD_MS = 2000;

const CREDENTIALS_STORAGE_KEY = 'broker_credentials';

function loadStoredCredentials(): Pick<BrokerSettings, 'username' | 'password'> {
	try {
		const raw = localStorage.getItem(CREDENTIALS_STORAGE_KEY);
		if (!raw) return { username: '', password: '' };
		const parsed = JSON.parse(raw) as Partial<BrokerSettings>;
		return { username: parsed.username ?? '', password: parsed.password ?? '' };
	} catch {
		return { username: '', password: '' };
	}
}

function saveCredentials(settings: BrokerSettings) {
	try {
		localStorage.setItem(
			CREDENTIALS_STORAGE_KEY,
			JSON.stringify({ username: settings.username, password: settings.password })
		);
	} catch {
		// Ignore storage quota/private mode errors.
	}
}

function clearStoredCredentials() {
	try {
		localStorage.removeItem(CREDENTIALS_STORAGE_KEY);
	} catch {
		// Ignore storage quota/private mode errors.
	}
}

function appendCommandLog(history: CommandLogEntry[], entry: CommandLogEntry) {
	return [entry, ...history].slice(0, COMMAND_HISTORY_LIMIT);
}

function sameCredentials(a: BrokerSettings | null, b: BrokerSettings) {
	if (!a) return false;
	return a.username === b.username && a.password === b.password;
}

function isAuthenticationError(message: string) {
	const normalized = message.toLowerCase();
	return (
		normalized.includes('not authorized') ||
		normalized.includes('bad username or password') ||
		normalized.includes('bad user name or password') ||
		normalized.includes('connection refused: 4') ||
		normalized.includes('connection refused: 5')
	);
}

// Read credentials synchronously at module evaluation time (browser only).
// This runs before any component renders, so hasCredentials is correct on
// the very first render — no login flash.
const INITIAL_CREDENTIALS = browser ? loadStoredCredentials() : { username: '', password: '' };

function createInitialState(): WaterAutomationState {
	const credentials = INITIAL_CREDENTIALS;
	const settings = createDefaultBrokerSettings(credentials);
	const url = settings.host ? buildBrokerUrl() : '';
	const configurationError = validateBrowserBrokerSettings();

	return {
		initialized: false,
		statusTopicSubscribed: false,
		telemetryReady: false,
		settings,
		wifiConnection: {
			wifiPhase: 'unknown',
			ssid: undefined,
			lastConnectedAt: undefined,
			lastError: undefined
		},
		arduinoMQTTConnection: { mqttPhase: 'unknown' },
		mqttConnection: {
			mqttPhase: configurationError ? 'error' : 'idle',
			detail:
				configurationError ??
				(settings.host ? `Ready to connect to ${url}.` : 'Set PUBLIC_MQTT_HOST in your .env file.'),
			url,
			lastError: configurationError ?? undefined
		},
		device: null,
		recentCommands: []
	};
}

function createWaterSystemStore() {
	const { subscribe, update } = writable<WaterAutomationState>(createInitialState());

	let client: MqttClient | null = null;
	let activeSettings: BrokerSettings | null = null;
	let initialized = false;
	let latestDevicePayload: ReturnType<typeof toDeviceTelemetry> | null = null;
	let latestPayloadFingerprint: string | null = null;
	let latestRawPayload: string | null = null;
	let applyRafId: number | null = null;
	let lastAppliedTelemetryAt = 0;

	function telemetryFingerprint(device: ReturnType<typeof toDeviceTelemetry>) {
		return [
			device.mode,
			device.override ? '1' : '0',
			device.overhead,
			device.sump,
			device.motors.borewell.status,
			device.motors.sump.status,
			device.alarms.emergencyStop ? '1' : '0',
			device.alarms.overheadCritical ? '1' : '0',
			device.alarms.sumpCritical ? '1' : '0',
			device.wifi_status ?? 'unknown',
			device.mqtt_status ?? 'unknown'
		].join('|');
	}

	function applyTelemetrySnapshot(settings: BrokerSettings) {
		if (!latestDevicePayload) return;
		perf.add('ui_apply_frames');
		const device = latestDevicePayload;
		const nextFingerprint = telemetryFingerprint(device);
		const sameFingerprint =
			latestPayloadFingerprint !== null && nextFingerprint === latestPayloadFingerprint;
		if (sameFingerprint && Date.now() - lastAppliedTelemetryAt < 1000) {
			perf.add('ui_apply_dropped');
			return;
		}
		latestPayloadFingerprint = nextFingerprint;
		lastAppliedTelemetryAt = Date.now();
		const startedAt = perf.enabled ? performance.now() : 0;
		update((state) => ({
			...state,
			device,
			telemetryReady: true,
			wifiConnection: device.wifi_status
				? { ...state.wifiConnection, wifiPhase: device.wifi_status }
				: state.wifiConnection,
			arduinoMQTTConnection: { mqttPhase: device.mqtt_status ?? 'unknown' },
			mqttConnection: {
				...state.mqttConnection,
				mqttPhase: 'connected',
				detail: `Receiving status from ${settings.statusTopic}.`,
				lastMessageAt: device.receivedAt,
				lastError: undefined
			}
		}));
		if (perf.enabled) perf.add('store_update_ms', performance.now() - startedAt);
		perf.add('store_updates');
	}

	function scheduleTelemetryApply(settings: BrokerSettings) {
		if (applyRafId !== null) return;
		applyRafId = requestAnimationFrame(() => {
			applyRafId = null;
			applyTelemetrySnapshot(settings);
		});
	}

	function closeClient() {
		latestRawPayload = null;
		latestDevicePayload = null;
		latestPayloadFingerprint = null;
		lastAppliedTelemetryAt = 0;
		const activeClient = client;
		client = null;
		activeSettings = null;
		if (activeClient) {
			activeClient.removeAllListeners();
			activeClient.end(true);
		}
		if (applyRafId !== null) {
			cancelAnimationFrame(applyRafId);
			applyRafId = null;
		}
	}

	async function connect() {
		if (!browser) return;

		const settings = sanitizeBrokerSettings(get({ subscribe }).settings);
		const configurationError = validateBrowserBrokerSettings();
		const url = settings.host ? buildBrokerUrl() : '';

		if (configurationError) {
			update((state) => ({
				...state,
				mqttConnection: {
					...state.mqttConnection,
					mqttPhase: 'error',
					detail: configurationError,
					url,
					lastError: configurationError
				}
			}));
			return;
		}

		if (client?.connected && sameCredentials(activeSettings, settings)) {
			update((state) => ({
				...state,
				mqttConnection: {
					...state.mqttConnection,
					mqttPhase: 'connected',
					detail: `Connected to ${url}.`,
					url,
					lastError: undefined
				}
			}));
			return;
		}

		closeClient();
		activeSettings = settings;

		update((state) => ({
			...state,
			settings,
			statusTopicSubscribed: false,
			telemetryReady: false,
			mqttConnection: {
				...state.mqttConnection,
				mqttPhase: 'connecting',
				detail: `Connecting to ${url}`,
				url,
				lastError: undefined
			}
		}));

		try {
			const nextClient = mqtt.connect(url, {
				username: settings.username || undefined,
				password: settings.password || undefined,
				clientId: createClientId(settings.clientIdPrefix),
				clean: true,
				connectTimeout: CONNECTION_TIMEOUT_MS,
				manualConnect: true,
				protocolVersion: 4,
				reconnectPeriod: RECONNECT_PERIOD_MS,
				resubscribe: true
			});

			client = nextClient;
			let reconnectAttempts = 0;
			let retriesExhausted = false;

			function failAndStop(detail: string, lastError?: string) {
				if (client !== nextClient || retriesExhausted) return;
				retriesExhausted = true;
				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					telemetryReady: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: 'error',
						detail,
						lastError: lastError ?? state.mqttConnection.lastError
					}
				}));
				closeClient();
			}

			nextClient.on('connect', () => {
				if (client !== nextClient) return;
				reconnectAttempts = 0;
				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					telemetryReady: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: 'connected',
						detail: `Connected. Subscribing to ${settings.statusTopic}.`,
						url,
						lastConnectedAt: Date.now(),
						lastError: undefined
					}
				}));
				nextClient.subscribe(settings.statusTopic, (error) => {
					if (client !== nextClient) return;
					if (error) {
						failAndStop('Connected, but subscription failed. Try again.', error.message);
						return;
					}
					update((state) => ({
						...state,
						statusTopicSubscribed: true,
						mqttConnection: {
							...state.mqttConnection,
							detail: `Receiving status from ${settings.statusTopic}.`
						}
					}));
				});
			});

			nextClient.on('reconnect', () => {
				if (client !== nextClient || retriesExhausted) return;
				reconnectAttempts += 1;
				if (reconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
					failAndStop(
						`Unable to reconnect after ${MAX_RECONNECT_ATTEMPTS} attempts.`,
						'Reconnect limit exceeded'
					);
					return;
				}
				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					telemetryReady: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: 'reconnecting',
						detail: `Retrying (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS})…`,
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('offline', () => {
				if (client !== nextClient || retriesExhausted) return;
				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					telemetryReady: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'offline',
						detail:
							reconnectAttempts > 0
								? `Retrying (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS})…`
								: 'Broker offline. Retrying automatically.',
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('close', () => {
				if (client !== nextClient || retriesExhausted) return;
				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					telemetryReady: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'offline',
						detail:
							reconnectAttempts > 0
								? `Retrying (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS})…`
								: 'Connection closed. Retrying automatically.',
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('error', (error) => {
				if (client !== nextClient || retriesExhausted) return;
				const message = error.message || 'MQTT client error';
				if (isAuthenticationError(message)) {
					retriesExhausted = true;
					clearCredentialsForReauth('Authentication failed. Check credentials and try again.');
					return;
				}
				update((state) => ({
					...state,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'connecting',
						detail:
							reconnectAttempts > 0
								? `Retry ${reconnectAttempts} of ${MAX_RECONNECT_ATTEMPTS} failed.`
								: 'Connection failed. Retrying automatically.',
						lastError: message
					}
				}));
			});

			nextClient.on('message', (topic, payload) => {
				if (client !== nextClient || topic !== settings.statusTopic) return;
				const rawPayload =
					typeof payload === 'string' ? payload : new TextDecoder().decode(payload);
				perf.add('mqtt_messages');
				if (latestRawPayload === rawPayload) {
					perf.add('ui_apply_dropped');
					return;
				}
				latestRawPayload = rawPayload;
				try {
					latestDevicePayload = perf.time(() => toDeviceTelemetry(rawPayload), 'mqtt_parse_ms');
					if (applyRafId !== null) perf.add('ui_apply_dropped');
					if (document.visibilityState === 'visible') {
						scheduleTelemetryApply(settings);
					} else {
						queueMicrotask(() => applyTelemetrySnapshot(settings));
					}
				} catch (error) {
					const message = error instanceof Error ? error.message : 'Unknown payload parse failure';
					console.warn('[system] MQTT payload parse failure:', message, rawPayload);
				}
			});

			nextClient.connect();
		} catch (error) {
			const message = error instanceof Error ? error.message : 'Failed to create MQTT client';
			closeClient();
			update((state) => ({
				...state,
				statusTopicSubscribed: false,
				telemetryReady: false,
				mqttConnection: {
					...state.mqttConnection,
					mqttPhase: 'error',
					detail: message,
					lastError: message
				}
			}));
		}
	}

	function sendCommand(command: ArduinoCommand) {
		const at = Date.now();
		const id = createCommandLogId(command, at);
		const topic = activeSettings?.commandTopic ?? get({ subscribe }).settings.commandTopic;
		if (!client || !client.connected || !activeSettings) {
			update((state) => ({
				...state,
				recentCommands: appendCommandLog(state.recentCommands, {
					id,
					at,
					command,
					status: 'failed',
					error: 'Broker is not connected'
				})
			}));
			return;
		}
		client.publish(topic, command, (error) => {
			update((state) => ({
				...state,
				recentCommands: appendCommandLog(state.recentCommands, {
					id,
					at,
					command,
					status: error ? 'failed' : 'sent',
					error: error?.message
				}),
				mqttConnection: error
					? {
							...state.mqttConnection,
							mqttPhase: 'error',
							detail: `Failed to publish ${command}.`,
							lastError: error.message
						}
					: state.mqttConnection
			}));
		});
	}

	function initialize() {
		if (!browser || initialized) {
			if (!browser) update((state) => ({ ...state, initialized: true }));
			return;
		}
		initialized = true;
		perf.start();

		// INITIAL_CREDENTIALS was already read at module load time, so settings
		// are already correct in the store. We just flip the flag and connect.
		update((state) => ({ ...state, initialized: true }));

		const credentials = INITIAL_CREDENTIALS;
		const hasCreds = !!(credentials.username || credentials.password);
		const settings = get({ subscribe }).settings;
		const configurationError = validateBrowserBrokerSettings();

		if (!configurationError && settings.host && hasCreds) {
			void connect();
		}
	}

	function updateSettings(next: BrokerSettings) {
		saveCredentials(next);
		update((state) => ({
			...state,
			settings: createDefaultBrokerSettings({ username: next.username, password: next.password })
		}));
		void connect();
	}

	function clearCredentials() {
		closeClient();
		clearStoredCredentials();
		update((state) => ({
			...state,
			device: null,
			telemetryReady: false,
			statusTopicSubscribed: false,
			settings: createDefaultBrokerSettings({ username: '', password: '' }),
			mqttConnection: {
				...state.mqttConnection,
				mqttPhase: 'idle',
				detail: 'Credentials cleared.',
				lastError: undefined
			}
		}));
	}

	function clearCredentialsForReauth(errorMessage: string) {
		closeClient();
		clearStoredCredentials();
		update((state) => ({
			...state,
			device: null,
			telemetryReady: false,
			statusTopicSubscribed: false,
			settings: createDefaultBrokerSettings({ username: '', password: '' }),
			mqttConnection: {
				...state.mqttConnection,
				mqttPhase: 'error',
				detail: 'Broker authentication failed. Enter credentials again.',
				lastError: errorMessage
			}
		}));
	}

	return { subscribe, initialize, sendCommand, updateSettings, clearCredentials };
}

export const waterSystem = createWaterSystemStore();

// Derived from the eagerly-read INITIAL_CREDENTIALS so it is correct on the
// very first render, before initialize() is ever called.
export const hasCredentials = derived(
	waterSystem,
	($s) => !!($s.settings.username || $s.settings.password)
);

export const deviceTelemetry = derived(waterSystem, ($s) => $s.device);
export const mqttConnectionState = derived(waterSystem, ($s) => $s.mqttConnection);
export const wifiConnectionState = derived(waterSystem, ($s) => $s.wifiConnection);
export const arduinoMqttConnectionState = derived(waterSystem, ($s) => $s.arduinoMQTTConnection);
