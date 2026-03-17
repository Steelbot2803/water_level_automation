import { Globe, GlobeOff, GlobeX, Hourglass, LoaderCircle } from 'lucide-svelte';

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
	'force borewell',
	'force sump',
	'unlock borewell',
	'unlock sump',
	'estop',
	'resume'
] as const;

export type ArduinoCommand = (typeof arduinoCommands)[number];

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
	clientIdPrefix: string;
}

export type ConnectionPhase =
	| 'idle'
	| 'connecting'
	| 'connected'
	| 'reconnecting'
	| 'offline'
	| 'error';

export const connectionIcons = [
	{ val: 'idle', icon: Hourglass },
	{ val: 'connecting', icon: LoaderCircle },
	{ val: 'connected', icon: Globe },
	{ val: 'reconnecting', icon: LoaderCircle },
	{ val: 'offline', icon: GlobeOff },
	{ val: 'error', icon: GlobeX }
] as const;

export const connectionIconsMap: Record<ConnectionPhase, (typeof connectionIcons)[number]['icon']> =
	Object.fromEntries(connectionIcons.map(({ val, icon }) => [val, icon])) as Record<
		ConnectionPhase,
		(typeof connectionIcons)[number]['icon']
	>;

export interface BrokerConnectionState {
	phase: ConnectionPhase;
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
	settings: BrokerSettings;
	connection: BrokerConnectionState;
	device: DeviceTelemetry | null;
	recentCommands: CommandLogEntry[];
}
