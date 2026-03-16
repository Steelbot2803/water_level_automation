import { writable } from 'svelte/store';

type PumpPreference = 'sump' | 'borewell';

function createPumpPreference() {
	const { subscribe, set } = writable<PumpPreference>('sump', (set) => {
		if (typeof window !== 'undefined') {
			const storedPreference = localStorage.getItem('pump_preference');
			if (storedPreference) {
				set(storedPreference as PumpPreference);
			}
		}
	});

	return {
		subscribe,
		set: (value: PumpPreference) => {
			if (typeof window !== 'undefined') {
				localStorage.setItem('pump_preference', value);
			}
			set(value);
		},
		toggle: () => {
			if (typeof window !== 'undefined') {
				const currentPreference = localStorage.getItem('pump_preference');
				const newPreference = currentPreference === 'sump' ? 'borewell' : 'sump';
				localStorage.setItem('pump_preference', newPreference);
				set(newPreference);
			}
		}
	};
}

export const pumpPreference = createPumpPreference();
