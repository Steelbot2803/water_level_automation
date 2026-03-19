import { writable } from 'svelte/store';

export interface NotificationPreferences {
	overheadCritical: boolean;
	overheadLow: boolean;
	overheadFull: boolean;
	sumpCritical: boolean;
	borewellDryRun: boolean;
	sumpDryRun: boolean;
	emergencyStop: boolean;
	connectionLost: boolean;
}

export const notificationLabels: Record<keyof NotificationPreferences, string> = {
	overheadCritical: 'Overhead tank critical',
	overheadLow: 'Overhead tank low',
	overheadFull: 'Overhead tank full',
	sumpCritical: 'Sump tank critical',
	borewellDryRun: 'Borewell dry-run lock',
	sumpDryRun: 'Sump dry-run lock',
	emergencyStop: 'Emergency stop activated',
	connectionLost: 'Broker connection lost'
};

const STORAGE_KEY = 'notification_preferences';

const defaults: NotificationPreferences = {
	overheadCritical: true,
	overheadLow: true,
	overheadFull: true,
	sumpCritical: true,
	borewellDryRun: true,
	sumpDryRun: true,
	emergencyStop: true,
	connectionLost: true
};

function createNotificationPreferences() {
	const { subscribe, set, update } = writable<NotificationPreferences>({ ...defaults });

	return {
		subscribe,
		initialize() {
			try {
				const stored = localStorage.getItem(STORAGE_KEY);
				if (stored) set({ ...defaults, ...JSON.parse(stored) });
			} catch {
				set({ ...defaults });
			}
		},
		toggle(key: keyof NotificationPreferences) {
			update((prefs) => {
				const next = { ...prefs, [key]: !prefs[key] };
				localStorage.setItem(STORAGE_KEY, JSON.stringify(next));
				return next;
			});
		}
	};
}

export const notificationPrefs = createNotificationPreferences();
