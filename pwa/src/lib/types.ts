export type Mode = 'AUTO' | 'OVERRIDE_FILL' | 'MANUAL';
export type Motor = 'BOREWELL' | 'SUMP';
export type MotorState = 'RUNNING' | 'STOPPED' | 'FAULT_DRY_RUN' | 'LOCKOUT';

export type OverheadLevel = 'CRITICAL' | 'LOW' | 'MEDIUM' | 'HIGH';
export type SumpLevel = 'BELOW_CRITICAL' | 'CRITICAL' | 'LOW' | 'HIGH';

export interface MotorRuntime {
	commandedOn: boolean;
	status: MotorState;
	dryRunDetected: boolean;
	lockoutUntil?: number;
	runtimeSeconds: number;
}

export interface Alarms {
	overheadCritical: boolean;
	sumpCritical: boolean;
	sumpBelowCritical: boolean;
}

export interface WaterAutomationState {
	mode: Mode;
	overhead: OverheadLevel;
	sump: SumpLevel;
	activeMotor: Motor | null;
	needFill: boolean;
	alarms: Alarms;
	cycleCount: number;
	lastEvent: string;
	motors: Record<Motor, MotorRuntime>;
}

export interface ControlCommand {
	type:
		| 'SET_MODE'
		| 'RUN_AUTOMATION'
		| 'FORCE_FILL'
		| 'STOP_ALL'
		| 'SIMULATE_MINUTE'
		| 'SET_OVERHEAD_LEVEL'
		| 'SET_SUMP_LEVEL'
		| 'SET_MOTOR_DRY_RUN'
		| 'SET_MANUAL_MOTOR';
	payload?: string | boolean;
}
