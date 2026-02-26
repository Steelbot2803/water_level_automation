import type { Motor, OverheadLevel, SumpLevel, WaterAutomationState } from './types.js';

const overheadFillLevels: OverheadLevel[] = ['CRITICAL', 'LOW'];

function motorIsLocked(state: WaterAutomationState, motor: Motor, now = Date.now()) {
	const lockout = state.motors[motor].lockoutUntil;
	return Boolean(lockout && lockout > now);
}

function motorIsBlocked(state: WaterAutomationState, motor: Motor) {
	if (state.motors[motor].dryRunDetected) {
		return `${motor} dry run detected`;
	}

	if (motorIsLocked(state, motor)) {
		return `${motor} in lockout`;
	}

	if (motor === 'SUMP' && state.mode !== 'MANUAL' && state.sump === 'BELOW_CRITICAL') {
		return 'SUMP blocked: sump tank below critical';
	}

	return null;
}

function chooseMotor(state: WaterAutomationState) {
	const motorPriority: Motor[] = ['BOREWELL', 'SUMP'];
	for (const motor of motorPriority) {
		const blockedReason = motorIsBlocked(state, motor);
		if (!blockedReason) {
			return { motor, blockedReason: null };
		}
	}
	return { motor: null, blockedReason: 'No motor available due to safety constraints' };
}

function stopAllMotors(state: WaterAutomationState) {
	state.activeMotor = null;
	state.motors.BOREWELL.commandedOn = false;
	state.motors.BOREWELL.status = state.motors.BOREWELL.dryRunDetected ? 'FAULT_DRY_RUN' : 'STOPPED';
	state.motors.SUMP.commandedOn = false;
	state.motors.SUMP.status = state.motors.SUMP.dryRunDetected ? 'FAULT_DRY_RUN' : 'STOPPED';
}

function startMotor(state: WaterAutomationState, motor: Motor) {
	state.activeMotor = motor;
	state.motors.BOREWELL.commandedOn = motor === 'BOREWELL';
	state.motors.BOREWELL.status =
		motor === 'BOREWELL'
			? 'RUNNING'
			: state.motors.BOREWELL.dryRunDetected
				? 'FAULT_DRY_RUN'
				: 'STOPPED';
	state.motors.SUMP.commandedOn = motor === 'SUMP';
	state.motors.SUMP.status =
		motor === 'SUMP' ? 'RUNNING' : state.motors.SUMP.dryRunDetected ? 'FAULT_DRY_RUN' : 'STOPPED';
}

export function runAutomation(state: WaterAutomationState): WaterAutomationState {
	const next = structuredClone(state);
	next.cycleCount += 1;
	next.alarms = {
		overheadCritical: next.overhead === 'CRITICAL',
		sumpCritical: next.sump === 'CRITICAL',
		sumpBelowCritical: next.sump === 'BELOW_CRITICAL'
	};

	next.needFill = next.mode === 'OVERRIDE_FILL' || overheadFillLevels.includes(next.overhead);

	if (next.overhead === 'HIGH') {
		stopAllMotors(next);
		next.lastEvent = 'Overhead tank at HIGH: stopped all motors';
		if (next.mode === 'OVERRIDE_FILL') {
			next.mode = 'AUTO';
			next.lastEvent += '; mode switched to AUTO';
		}
		return next;
	}

	if (next.mode === 'MANUAL') {
		next.lastEvent = 'Manual mode active: automatic selection skipped';
		return next;
	}

	if (!next.needFill) {
		stopAllMotors(next);
		next.lastEvent = 'No fill demand: stopped all motors';
		return next;
	}

	const { motor, blockedReason } = chooseMotor(next);

	if (!motor) {
		stopAllMotors(next);
		next.lastEvent = blockedReason ?? 'Motor selection failed';
		return next;
	}

	startMotor(next, motor);
	next.lastEvent = `${motor} running (${next.mode})`;
	return next;
}

export function simulateMinute(state: WaterAutomationState): WaterAutomationState {
	const next = structuredClone(state);

	if (!next.activeMotor) {
		next.lastEvent = 'No active motor to simulate';
		return next;
	}

	next.motors[next.activeMotor].runtimeSeconds += 60;

	const overheadProgress: Record<OverheadLevel, OverheadLevel> = {
		CRITICAL: 'LOW',
		LOW: 'MEDIUM',
		MEDIUM: 'HIGH',
		HIGH: 'HIGH'
	};
	next.overhead = overheadProgress[next.overhead];

	if (next.activeMotor === 'SUMP') {
		const sumpDrain: Record<SumpLevel, SumpLevel> = {
			HIGH: 'LOW',
			LOW: 'CRITICAL',
			CRITICAL: 'BELOW_CRITICAL',
			BELOW_CRITICAL: 'BELOW_CRITICAL'
		};
		next.sump = sumpDrain[next.sump];
	}

	if (next.overhead === 'HIGH') {
		stopAllMotors(next);
		next.lastEvent = 'Fill cycle complete: overhead reached HIGH';
		next.motors.BOREWELL.commandedOn = false;
		next.motors.SUMP.commandedOn = false;
	}

	if (next.mode !== 'MANUAL' && next.activeMotor === 'SUMP' && next.sump === 'BELOW_CRITICAL') {
		next.activeMotor = null;
		next.motors.SUMP.commandedOn = false;
		next.motors.SUMP.status = 'STOPPED';
		next.lastEvent = 'SUMP stopped: sump dropped below critical';
	}

	return next;
}
