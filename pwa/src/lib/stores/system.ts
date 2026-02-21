import { writable } from 'svelte/store';
import { runAutomation, simulateMinute } from '../control.js';
import type { Mode, Motor, OverheadLevel, SumpLevel, WaterAutomationState } from '../types.js';

const initialState: WaterAutomationState = {
	mode: 'AUTO',
	overhead: 'MEDIUM',
	sump: 'HIGH',
	activeMotor: null,
	needFill: false,
	alarms: {
		overheadCritical: false,
		sumpCritical: false,
		sumpBelowCritical: false
	},
	cycleCount: 0,
	lastEvent: 'System boot complete',
	motors: {
		BOREWELL: {
			commandedOn: false,
			status: 'STOPPED',
			dryRunDetected: false,
			runtimeSeconds: 0
		},
		SUMP: {
			commandedOn: false,
			status: 'STOPPED',
			dryRunDetected: false,
			runtimeSeconds: 0
		}
	}
};

function createWaterSystemStore() {
	const { subscribe, update, set } = writable<WaterAutomationState>(initialState);

	return {
		subscribe,
		reset: () => set(initialState),
		runAutomation: () => update((state) => runAutomation(state)),
		simulateMinute: () => update((state) => simulateMinute(state)),
		setMode: (mode: Mode) =>
			update((state) => ({
				...state,
				mode,
				lastEvent: `Mode changed to ${mode}`
			})),
		forceFill: () =>
			update((state) =>
				runAutomation({
					...state,
					mode: 'OVERRIDE_FILL',
					lastEvent: 'Force fill requested'
				})
			),
		stopAll: () =>
			update((state) => ({
				...state,
				activeMotor: null,
				motors: {
					...state.motors,
					BOREWELL: { ...state.motors.BOREWELL, commandedOn: false, status: 'STOPPED' },
					SUMP: { ...state.motors.SUMP, commandedOn: false, status: 'STOPPED' }
				},
				lastEvent: 'Emergency stop: all motors stopped'
			})),
		setOverheadLevel: (level: OverheadLevel) =>
			update((state) => ({ ...state, overhead: level, lastEvent: `Overhead set to ${level}` })),
		setSumpLevel: (level: SumpLevel) =>
			update((state) => ({ ...state, sump: level, lastEvent: `Sump set to ${level}` })),
		setDryRun: (motor: Motor, dryRunDetected: boolean) =>
			update((state) => ({
				...state,
				motors: {
					...state.motors,
					[motor]: {
						...state.motors[motor],
						dryRunDetected,
						status: dryRunDetected ? 'FAULT_DRY_RUN' : 'STOPPED'
					}
				},
				lastEvent: `${motor} dry-run sensor ${dryRunDetected ? 'TRIGGERED' : 'CLEARED'}`
			})),
		manualStart: (motor: Motor) =>
			update((state) => {
				if (state.mode !== 'MANUAL') {
					return { ...state, lastEvent: 'Ignored manual start: not in MANUAL mode' };
				}

				if (state.motors[motor].dryRunDetected) {
					return { ...state, lastEvent: `Cannot start ${motor}: dry run fault active` };
				}

				return {
					...state,
					activeMotor: motor,
					motors: {
						...state.motors,
						BOREWELL: {
							...state.motors.BOREWELL,
							commandedOn: motor === 'BOREWELL',
							status: motor === 'BOREWELL' ? 'RUNNING' : 'STOPPED'
						},
						SUMP: {
							...state.motors.SUMP,
							commandedOn: motor === 'SUMP',
							status: motor === 'SUMP' ? 'RUNNING' : 'STOPPED'
						}
					},
					lastEvent: `Manual start: ${motor}`
				};
			})
	};
}

export const waterSystem = createWaterSystemStore();
