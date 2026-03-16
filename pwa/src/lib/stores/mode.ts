import { writable } from 'svelte/store';

type Mode = 'auto' | 'manual';

function createMode() {
	const { subscribe, set } = writable<Mode>('auto');
	let seededFromDevice = false;

	return {
		subscribe,
		set,
		seedFromDevice: (incoming: Mode) => {
			if (seededFromDevice) return;
			seededFromDevice = true;
			set(incoming);
		}
	};
}

export const mode = createMode();
