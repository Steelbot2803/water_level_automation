import {
	CircleAlert,
	Globe,
	GlobeOff,
	GlobeX,
	Hourglass,
	LoaderCircle,
	Server,
	ServerOff,
	Wifi,
	WifiOff
} from 'lucide-svelte';

export const mqttModes = ['auto', 'manual'] as const;
export type MqttMode = (typeof mqttModes)[number];

export const controlModes = ['auto', 'override_fill', 'manual'] as const;
export type ControlMode = (typeof controlModes)[number];

export const overheadLevels = ['critical', 'low', 'medium', 'high'] as const;
export type OverheadLevel = (typeof overheadLevels)[number];

export const sumpLevels = ['critical', 'low', 'high'] as const;
export type SumpLevel = (typeof sumpLevels)[number];

export const activeMotors = ['none', 'borewell', 'sump'] as const;
export type ActiveMotor = (typeof activeMotors)[number];

export const motorRuntimeStatuses = [
	'stopped',
	'starting',
	'running',
	'dry_run_lock',
	'sump_critical'
] as const;
export type MotorRuntimeStatus = (typeof motorRuntimeStatuses)[number];

export const arduinoCommands = [
	'auto',
	'manual',
	'override',
	'motor borewell',
	'motor sump',
	'motor stop',
	'borewell',
	'sump',
	'unlock borewell',
	'unlock sump',
	'estop',
	'resume',
	'reset',
	'reset state'
] as const;
export type ArduinoCommand = (typeof arduinoCommands)[number];

export const wifiConnectionPhases = [
	'unknown',
	'connected',
	'connecting',
	'disconnected',
	'connection_lost',
	'connect_failed',
	'ssid_unavailable',
	'no_module'
] as const;
export type WifiConnectionPhase = (typeof wifiConnectionPhases)[number];

export const mqttConnectionPhases = [
	'idle',
	'connecting',
	'connected',
	'reconnecting',
	'offline',
	'error'
] as const;
export type MQTTConnectionPhase = (typeof mqttConnectionPhases)[number];

export const arduinoMQTTConnectionPhases = ['unknown', 'connected', 'disconnected'] as const;
export type ArduinoMQTTConnectionPhase = (typeof arduinoMQTTConnectionPhases)[number];

export interface ArduinoStatusPayload {
	mode: MqttMode;
	override: boolean;
	overhead: OverheadLevel;
	sump: SumpLevel;
	motor: ActiveMotor;
	manual_target?: ActiveMotor;
	borewell_status?: MotorRuntimeStatus;
	sump_status?: MotorRuntimeStatus;
	sump_warning?: boolean;
	emergency_stop?: boolean;
	auto_prefer_sump?: boolean;
	wifi_status?: WifiConnectionPhase;
	mqtt_status?: ArduinoMQTTConnectionPhase;
}

export interface ArduinoHeartbeat {
	seq: number;
	wifi: WifiConnectionPhase;
	mqtt: boolean;
}

export interface WifiConnectionState {
	wifiPhase: WifiConnectionPhase;
	ssid?: string;
	lastConnectedAt?: number;
	lastError?: string;
}

export interface ArduinoConnectionStatusPayload {
	mqttPhase: ArduinoMQTTConnectionPhase;
}

export interface DeviceAlarms {
	overheadCritical: boolean;
	sumpCritical: boolean;
	emergencyStop: boolean;
}

export interface MotorTelemetry {
	active: boolean;
	status: MotorRuntimeStatus;
}

export interface DeviceTelemetry extends ArduinoStatusPayload {
	controlMode: ControlMode;
	needFill: boolean;
	receivedAt: number;
	rawPayload: string;
	alarms: DeviceAlarms;
	motors: {
		borewell: MotorTelemetry;
		sump: MotorTelemetry;
	};
}

export interface BrokerSettings {
	host: string;
	port: string;
	path: string;
	username: string;
	password: string;
	useSSL: boolean;
	commandTopic: string;
	statusTopic: string;
	heartbeatTopic: string;
	clientIdPrefix: string;
}

export const wifiConnectionIcons = [
	{ val: 'unknown', icon: Hourglass },
	{ val: 'connected', icon: Wifi },
	{ val: 'connecting', icon: LoaderCircle },
	{ val: 'disconnected', icon: WifiOff },
	{ val: 'connection_lost', icon: WifiOff },
	{ val: 'connect_failed', icon: CircleAlert },
	{ val: 'ssid_unavailable', icon: CircleAlert },
	{ val: 'no_module', icon: CircleAlert }
] as const;

export const wifiConnectionIconsMap: Record<
	WifiConnectionPhase,
	(typeof wifiConnectionIcons)[number]['icon']
> = Object.fromEntries(wifiConnectionIcons.map(({ val, icon }) => [val, icon])) as Record<
	WifiConnectionPhase,
	(typeof wifiConnectionIcons)[number]['icon']
>;

export const mqttConnectionIcons = [
	{ val: 'idle', icon: Hourglass },
	{ val: 'connecting', icon: LoaderCircle },
	{ val: 'connected', icon: Globe },
	{ val: 'reconnecting', icon: LoaderCircle },
	{ val: 'offline', icon: GlobeOff },
	{ val: 'error', icon: GlobeX }
] as const;

export const mqttConnectionIconsMap: Record<
	MQTTConnectionPhase,
	(typeof mqttConnectionIcons)[number]['icon']
> = Object.fromEntries(mqttConnectionIcons.map(({ val, icon }) => [val, icon])) as Record<
	MQTTConnectionPhase,
	(typeof mqttConnectionIcons)[number]['icon']
>;

export const arduinoMQTTConnectionIcons = [
	{ val: 'unknown', icon: Hourglass },
	{ val: 'connected', icon: Server },
	{ val: 'disconnected', icon: ServerOff }
] as const;

export const arduinoMQTTConnectionIconsMap: Record<
	ArduinoMQTTConnectionPhase,
	(typeof arduinoMQTTConnectionIcons)[number]['icon']
> = Object.fromEntries(arduinoMQTTConnectionIcons.map(({ val, icon }) => [val, icon])) as Record<
	ArduinoMQTTConnectionPhase,
	(typeof arduinoMQTTConnectionIcons)[number]['icon']
>;

export interface BrokerConnectionState {
	mqttPhase: MQTTConnectionPhase;
	detail: string;
	url: string;
	lastConnectedAt?: number;
	lastMessageAt?: number;
	lastError?: string;
}

export interface CommandLogEntry {
	id: string;
	at: number;
	command: ArduinoCommand;
	status: 'sent' | 'failed';
	error?: string;
}

export interface WaterAutomationState {
	initialized: boolean;
	statusTopicSubscribed: boolean;
	telemetryReady: boolean;
	settings: BrokerSettings;
	wifiConnection: WifiConnectionState;
	arduinoMQTTConnection: ArduinoConnectionStatusPayload;
	mqttConnection: BrokerConnectionState;
	device: DeviceTelemetry | null;
	recentCommands: CommandLogEntry[];
	lastHeartbeatSeq: number | null;
	lastHeartbeatAt: number | null;
}
