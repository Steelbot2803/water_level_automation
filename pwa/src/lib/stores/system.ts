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
		connection: {
			phase: configurationError ? 'error' : 'idle',
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
				connection: {
					...state.connection,
					phase: 'error',
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
				connection: {
					...state.connection,
					phase: 'connected',
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
			connection: {
				...state.connection,
				phase: 'connecting',
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
				connectTimeout: 10000,
				manualConnect: true,
				protocolVersion: 4,
				reconnectPeriod: 5000,
				resubscribe: true
			});

			client = nextClient;

			let connectionTimeout: ReturnType<typeof setTimeout> | null = setTimeout(() => {
				if (client !== nextClient) return;

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					connection: {
						...state.connection,
						phase: 'error',
						detail: 'Broker connection timed out. Check the browser console and verify WebSocket access to HiveMQ.',
						lastError: `No MQTT event received within ${CONNECTION_TIMEOUT_MS / 1000}s`
					}
				}));

				nextClient.end(true);
			}, CONNECTION_TIMEOUT_MS);

			function clearConnectionTimeout() {
				if (connectionTimeout === null) return;
				clearTimeout(connectionTimeout);
				connectionTimeout = null;
			}

			nextClient.on('connect', () => {
				if (client !== nextClient) return;
				clearConnectionTimeout();

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					connection: {
						...state.connection,
						phase: 'connected',
						detail: `Connected to ${url}. Subscribing to ${settings.statusTopic}.`,
						url,
						lastConnectedAt: Date.now(),
						lastError: undefined
					}
				}));

				nextClient.subscribe(settings.statusTopic, (error) => {
					if (client !== nextClient) return;

					if (error) {
						update((state) => ({
							...state,
							statusTopicSubscribed: false,
							connection: {
								...state.connection,
								phase: 'error',
								detail: `Connected, but subscribe failed for ${settings.statusTopic}.`,
								lastError: error.message
							}
						}));
						return;
					}

					update((state) => ({
						...state,
						statusTopicSubscribed: true,
						connection: {
							...state.connection,
							detail: `Receiving status from ${settings.statusTopic}.`
						}
					}));
				});
			});

			nextClient.on('reconnect', () => {
				if (client !== nextClient) return;
				clearConnectionTimeout();

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					connection: {
						...state.connection,
						phase: 'reconnecting',
						detail: 'Broker connection lost. Retrying.',
						lastError: state.connection.lastError
					}
				}));
			});

			nextClient.on('offline', () => {
				if (client !== nextClient) return;
				clearConnectionTimeout();

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					connection: {
						...state.connection,
						phase: 'offline',
						detail: 'Broker connection is offline.',
						lastError: state.connection.lastError
					}
				}));
			});

			nextClient.on('close', () => {
				if (client !== nextClient) return;
				clearConnectionTimeout();

				update((state) => ({
					...state,
					statusTopicSubscribed: false,
					connection: {
						...state.connection,
						phase: 'offline',
						detail: 'Broker connection closed.',
						lastError: state.connection.lastError
					}
				}));
			});

			nextClient.on('error', (error) => {
				if (client !== nextClient) return;
				clearConnectionTimeout();

				update((state) => ({
					...state,
					connection: {
						...state.connection,
						phase: 'error',
						detail: error.message || 'MQTT client error',
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
						connection: {
							...state.connection,
							phase: 'connected',
							detail: `Receiving status from ${settings.statusTopic}.`,
							lastMessageAt: device.receivedAt,
							lastError: undefined
						}
					}));
				} catch (error) {
					const message = error instanceof Error ? error.message : 'Unknown payload parse failure';
					update((state) => ({
						...state,
						connection: {
							...state.connection,
							phase: 'error',
							detail: 'Received invalid MQTT status payload.',
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
				connection: {
					...state.connection,
					phase: 'error',
					detail: message,
					lastError: message
				}
			}));
		}
	}

	function disconnect() {
		closeClient();
		update((state) => ({
			...state,
			statusTopicSubscribed: false,
			connection: {
				...state.connection,
				phase: 'idle',
				detail: `Disconnected from ${state.connection.url || 'broker'}.`,
				lastError: undefined
			}
		}));
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
				connection: error
					? {
							...state.connection,
							phase: 'error',
							detail: `Failed to publish ${command} to ${topic}.`,
							lastError: error.message
						}
					: state.connection
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
			connection: {
				...state.connection,
				phase: configurationError ? 'error' : 'idle',
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
		connect,
		disconnect,
		sendCommand
	};
}

export const waterSystem = createWaterSystemStore();
