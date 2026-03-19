import { browser } from '$app/environment';
import mqtt, { type MqttClient } from 'mqtt';
import { get, writable } from 'svelte/store';

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
const CONNECTION_TIMEOUT_MS = 15000;
const MAX_RECONNECT_ATTEMPTS = 10;
const RECONNECT_PERIOD_MS = 5000;

function appendCommandLog(history: CommandLogEntry[], entry: CommandLogEntry) {
	return [entry, ...history].slice(0, COMMAND_HISTORY_LIMIT);
}

function sameSettings(a: BrokerSettings | null, b: BrokerSettings) {
	if (!a) return false;

	return (
		a.host === b.host &&
		a.port === b.port &&
		a.path === b.path &&
		a.username === b.username &&
		a.password === b.password &&
		a.useSSL === b.useSSL &&
		a.commandTopic === b.commandTopic &&
		a.statusTopic === b.statusTopic &&
		a.clientIdPrefix === b.clientIdPrefix
	);
}

function createInitialState(): WaterAutomationState {
	const settings = createDefaultBrokerSettings();
	const url = settings.host ? buildBrokerUrl(settings) : '';
	const configurationError = validateBrowserBrokerSettings(settings);

	return {
		initialized: false,
		statusTopicSubscribed: false,
		settings,
		wifiConnection: {
			wifiPhase: 'unknown',
			ssid: undefined,
			lastConnectedAt: undefined,
			lastError: undefined
		},
		arduinoMQTTConnection: {
			mqttPhase: 'unknown'
		},
		mqttConnection: {
			mqttPhase: configurationError ? 'error' : 'idle',
			detail:
				configurationError ??
				(settings.host
					? `Auto-connect is configured for ${url}.`
					: 'Set the HiveMQ WebSocket values in `pwa/.env` and reload the app.'),
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

	function closeClient() {
		if (!client) return;

		const activeClient = client;
		client = null;
		activeSettings = null;
		activeClient.removeAllListeners();
		activeClient.end(true);
	}

	async function connect() {
		if (!browser) return;

		const settings = sanitizeBrokerSettings(get({ subscribe }).settings);
		const configurationError = validateBrowserBrokerSettings(settings);
		const url = settings.host ? buildBrokerUrl(settings) : '';

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

		if (client?.connected && sameSettings(activeSettings, settings)) {
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
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: 'connected',
						detail: `Connected to ${url}. Subscribing to ${settings.statusTopic}.`,
						url,
						lastConnectedAt: Date.now(),
						lastError: undefined
					}
				}));

				nextClient.subscribe(settings.statusTopic, (error) => {
					if (client !== nextClient) return;

					if (error) {
						failAndStop(
							'Connected, but status subscription failed. Pull to refresh to try again.',
							error.message
						);
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
						`Unable to reconnect after ${MAX_RECONNECT_ATTEMPTS} attempts. Pull to refresh to try again.`,
						`Reconnect limit exceeded after ${MAX_RECONNECT_ATTEMPTS} attempts`
					);
					return;
				}

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: 'reconnecting',
						detail: `Retrying broker connection (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS}).`,
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('offline', () => {
				if (client !== nextClient || retriesExhausted) return;

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'offline',
						detail:
							reconnectAttempts > 0
								? `Retrying broker connection (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS}).`
								: 'Broker connection is offline. Retrying automatically.',
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('close', () => {
				if (client !== nextClient || retriesExhausted) return;

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'offline',
						detail:
							reconnectAttempts > 0
								? `Retrying broker connection (${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS}).`
								: 'Broker connection closed. Retrying automatically.',
						lastError: state.mqttConnection.lastError
					}
				}));
			});

			nextClient.on('error', (error) => {
				if (client !== nextClient || retriesExhausted) return;

				update((state) => ({
					...state,
					mqttConnection: {
						...state.mqttConnection,
						mqttPhase: reconnectAttempts > 0 ? 'reconnecting' : 'connecting',
						detail:
							reconnectAttempts > 0
								? `Retry ${reconnectAttempts} of ${MAX_RECONNECT_ATTEMPTS} failed. Retrying automatically.`
								: 'Initial broker connection failed. Retrying automatically.',
						lastError: error.message || 'MQTT client error'
					}
				}));
			});

			nextClient.on('message', (topic, payload) => {
				if (client !== nextClient || topic !== settings.statusTopic) return;

				const rawPayload =
					typeof payload === 'string' ? payload : new TextDecoder().decode(payload);

				try {
					const device = toDeviceTelemetry(rawPayload);
					update((state) => ({
						...state,
						device,
						wifiConnection: device.wifi_status
							? { ...state.wifiConnection, wifiPhase: device.wifi_status }
							: state.wifiConnection,
						arduinoMQTTConnection: {
							mqttPhase: device.mqtt_status ?? 'unknown'
						},
						mqttConnection: {
							...state.mqttConnection,
							mqttPhase: 'connected',
							detail: `Receiving status from ${settings.statusTopic}.`,
							lastMessageAt: device.receivedAt,
							lastError: undefined
						}
					}));
				} catch (error) {
					const message = error instanceof Error ? error.message : 'Unknown payload parse failure';
					console.warn('[system] MQTT payload parse failure:', message, rawPayload);
					update((state) => ({
						...state,
						mqttConnection: {
							...state.mqttConnection,
							detail: `Warning: received unparseable status payload. Waiting for next message.`,
							lastError: message
						}
					}));
				}
			});

			nextClient.connect();
		} catch (error) {
			const message = error instanceof Error ? error.message : 'Failed to create MQTT client';
			closeClient();
			update((state) => ({
				...state,
				statusTopicSubscribed: false,
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
			const entry: CommandLogEntry = {
				id,
				at,
				command,
				status: 'failed',
				error: 'Broker is not connected'
			};

			update((state) => ({
				...state,
				recentCommands: appendCommandLog(state.recentCommands, entry)
			}));
			return;
		}

		client.publish(topic, command, (error) => {
			const entry: CommandLogEntry = {
				id,
				at,
				command,
				status: error ? 'failed' : 'sent',
				error: error?.message
			};

			update((state) => ({
				...state,
				recentCommands: appendCommandLog(state.recentCommands, entry),
				mqttConnection: error
					? {
							...state.mqttConnection,
							mqttPhase: 'error',
							detail: `Failed to publish ${command} to ${topic}.`,
							lastError: error.message
						}
					: state.mqttConnection
			}));
		});
	}

	function initialize() {
		if (!browser || initialized) {
			if (!browser) {
				update((state) => ({ ...state, initialized: true }));
			}
			return;
		}

		initialized = true;
		const settings = sanitizeBrokerSettings(createDefaultBrokerSettings());
		const url = settings.host ? buildBrokerUrl(settings) : '';
		const configurationError = validateBrowserBrokerSettings(settings);

		update((state) => ({
			...state,
			initialized: true,
			settings,
			mqttConnection: {
				...state.mqttConnection,
				mqttPhase: configurationError ? 'error' : 'idle',
				detail:
					configurationError ??
					(settings.host
						? `Auto-connecting to ${url}.`
						: 'Set the HiveMQ WebSocket values in `pwa/.env` and reload the app.'),
				url,
				lastError: configurationError ?? undefined
			}
		}));

		if (!configurationError && settings.host) {
			void connect();
		}
	}

	return {
		subscribe,
		initialize,
		sendCommand
	};
}

export const waterSystem = createWaterSystemStore();
