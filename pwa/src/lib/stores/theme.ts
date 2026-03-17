import { writable } from 'svelte/store';
import { Sun, Moon, Monitor } from 'lucide-svelte';

// The three states a user can be in.
// 'system' = follow the OS, 'light'/'dark' = user has made a choice.
export type ThemePreference = 'light' | 'dark' | 'system';

const STORAGE_KEY = 'theme_preference';

// Checks the OS/browser dark mode preference.
function getSystemTheme(): 'light' | 'dark' {
	if (typeof window !== 'undefined' && window.matchMedia('(prefers-color-scheme: dark)').matches) {
		return 'dark';
	}
	return 'light';
}

// Adds or removes the .dark class on <html> based on the resolved preference.
// This is what actually makes Tailwind's dark: classes activate.
function applyTheme(pref: ThemePreference) {
	if (typeof document === 'undefined') return;
	const resolved = pref === 'system' ? getSystemTheme() : pref;
	document.documentElement.classList.toggle('dark', resolved === 'dark');
}

function createThemeStore() {
	// Always start with 'system' — safe for SSR.
	const { subscribe, set } = writable<ThemePreference>('system');

	return {
		subscribe,

		set: (value: ThemePreference) => {
			if (typeof localStorage !== 'undefined') {
				if (value === 'system') {
					localStorage.removeItem(STORAGE_KEY);
				} else {
					localStorage.setItem(STORAGE_KEY, value);
				}
			}
			applyTheme(value);
			set(value);
		},

		initialize: (): (() => void) => {
			// ✅ NOW we're in the browser — safe to read localStorage.
			const stored = localStorage.getItem(STORAGE_KEY) as ThemePreference | null;
			const resolved: ThemePreference = stored ?? 'system';

			applyTheme(resolved);
			set(resolved); // ✅ Update the store with the real saved value.

			const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
			const handleSystemChange = () => {
				if (!localStorage.getItem(STORAGE_KEY)) {
					applyTheme('system');
				}
			};
			mediaQuery.addEventListener('change', handleSystemChange);
			return () => mediaQuery.removeEventListener('change', handleSystemChange);
		}
	};
}

export const theme = createThemeStore();

export const themeIcons = [
	{ val: 'light', icon: Sun },
	{ val: 'system', icon: Monitor },
	{ val: 'dark', icon: Moon }
];
