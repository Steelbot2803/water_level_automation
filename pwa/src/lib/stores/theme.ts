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
	// On first load, check localStorage. If nothing saved, default to 'system'.
	const stored =
		typeof localStorage !== 'undefined'
			? (localStorage.getItem(STORAGE_KEY) as ThemePreference | null)
			: null;
	const initial: ThemePreference = stored ?? 'system';

	const { subscribe, set } = writable<ThemePreference>(initial);

	return {
		subscribe,

		// Called when the user explicitly picks a theme.
		set: (value: ThemePreference) => {
			if (typeof localStorage !== 'undefined') {
				// Remove the key entirely for 'system' — that way, if they clear
				// localStorage, the app gracefully falls back to system default.
				if (value === 'system') {
					localStorage.removeItem(STORAGE_KEY);
				} else {
					localStorage.setItem(STORAGE_KEY, value);
				}
			}
			applyTheme(value);
			set(value);
		},

		// Called once in onMount. Applies the initial theme and starts
		// listening for OS-level dark/light changes (e.g. user switches
		// between day and night mode on their phone).
		initialize: (): (() => void) => {
			applyTheme(initial);

			if (typeof window !== 'undefined') {
				const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
				const handleSystemChange = () => {
					// Only react if the user hasn't pinned a preference.
					if (!localStorage.getItem(STORAGE_KEY)) {
						applyTheme('system');
					}
				};
				mediaQuery.addEventListener('change', handleSystemChange);
				// Returns a cleanup function so Svelte can remove the listener.
				return () => mediaQuery.removeEventListener('change', handleSystemChange);
			}

			return () => {};
		}
	};
}

export const theme = createThemeStore();

export const themeIcons = [
		{ val: 'light', icon: Sun },
		{ val: 'system', icon: Monitor },
		{ val: 'dark', icon: Moon }
	];
