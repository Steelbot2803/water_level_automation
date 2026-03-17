import { browser } from '$app/environment';
import { writable } from 'svelte/store';

export type AlertSeverity = 'success' | 'error' | 'info' | 'warning';

export interface AlertToast {
	id: number;
	title: string;
	message: string;
	severity: AlertSeverity;
	duration: number;
	remaining: number;
	timer?: ReturnType<typeof setTimeout>;
	startedAt?: number;
}

export interface AlertPayload {
	title: string;
	message: string;
	severity?: AlertSeverity;
	duration?: number;
	tag?: string;
	cooldownMs?: number;
}

const DEFAULT_DURATION_MS = 5000;
const DEFAULT_COOLDOWN_MS = 60000;
const MAX_TOASTS = 4;
const MOBILE_MEDIA_QUERY = '(max-width: 768px), (pointer: coarse)';

const { subscribe, update } = writable<AlertToast[]>([]);

let alertIdCounter = 0;
let initialized = false;
let mobileLike = false;
let notificationPermission: NotificationPermission = 'default';
let serviceWorkerRegistration: Promise<ServiceWorkerRegistration | null> | null = null;
const lastAlertByTag = new Map<string, number>();

function startTimer(id: number) {
	update((alerts) =>
		alerts.map((alert) => {
			if (alert.id !== id) return alert;

			return {
				...alert,
				startedAt: Date.now(),
				timer: setTimeout(() => dismiss(id), alert.remaining)
			};
		})
	);
}

function enqueueToast(
	payload: Required<Pick<AlertPayload, 'title' | 'message' | 'severity' | 'duration'>>
) {
	const id = (alertIdCounter += 1);
	const nextAlert: AlertToast = {
		id,
		title: payload.title,
		message: payload.message,
		severity: payload.severity,
		duration: payload.duration,
		remaining: payload.duration
	};

	update((alerts) => [...alerts.slice(-MAX_TOASTS + 1), nextAlert]);
	startTimer(id);
}

function dismiss(id: number) {
	update((alerts) => {
		const active = alerts.find((alert) => alert.id === id);
		if (active?.timer) {
			clearTimeout(active.timer);
		}

		return alerts.filter((alert) => alert.id !== id);
	});
}

function pause(id: number) {
	update((alerts) =>
		alerts.map((alert) => {
			if (alert.id !== id || !alert.timer || !alert.startedAt) return alert;

			clearTimeout(alert.timer);
			return {
				...alert,
				timer: undefined,
				remaining: Math.max(0, alert.remaining - (Date.now() - alert.startedAt)),
				startedAt: undefined
			};
		})
	);
}

function resume(id: number) {
	startTimer(id);
}

function refreshEnvironmentState() {
	if (!browser) return;

	mobileLike = window.matchMedia(MOBILE_MEDIA_QUERY).matches;
	notificationPermission = 'Notification' in window ? Notification.permission : 'denied';
}

async function showSystemNotification(
	payload: Required<Pick<AlertPayload, 'title' | 'message' | 'severity' | 'tag'>>
) {
	if (!browser || !('Notification' in window) || notificationPermission !== 'granted') {
		return false;
	}

	const options: NotificationOptions = {
		body: payload.message,
		tag: payload.tag,
		icon: '/favicon.svg',
		badge: '/favicon.svg',
		data: { severity: payload.severity }
	};

	try {
		const registration = serviceWorkerRegistration ? await serviceWorkerRegistration : null;
		if (registration) {
			await registration.showNotification(payload.title, options);
			return true;
		}

		new Notification(payload.title, options);
		return true;
	} catch {
		return false;
	}
}

function shouldSuppress(tag: string, cooldownMs: number) {
	const now = Date.now();
	const lastShownAt = lastAlertByTag.get(tag);

	if (lastShownAt && now - lastShownAt < cooldownMs) {
		return true;
	}

	lastAlertByTag.set(tag, now);
	return false;
}

async function requestNotificationPermission() {
	if (
		!browser ||
		!mobileLike ||
		!('Notification' in window) ||
		Notification.permission !== 'default'
	) {
		return;
	}

	try {
		notificationPermission = await Notification.requestPermission();
	} catch {
		notificationPermission = Notification.permission;
	}
}

function initialize() {
	if (!browser || initialized) {
		return () => {};
	}

	initialized = true;
	refreshEnvironmentState();

	const mediaQuery = window.matchMedia(MOBILE_MEDIA_QUERY);
	const handleMediaChange = () => refreshEnvironmentState();

	mediaQuery.addEventListener('change', handleMediaChange);

	if ('serviceWorker' in navigator) {
		serviceWorkerRegistration = navigator.serviceWorker
			.register('/service-worker.js')
			.then(() => navigator.serviceWorker.ready)
			.catch(() => null);
	}

	const handleFirstGesture = () => {
		void requestNotificationPermission();
	};

	window.addEventListener('pointerdown', handleFirstGesture, { once: true, passive: true });
	window.addEventListener('keydown', handleFirstGesture, { once: true });

	return () => {
		mediaQuery.removeEventListener('change', handleMediaChange);

		window.removeEventListener('pointerdown', handleFirstGesture);
		window.removeEventListener('keydown', handleFirstGesture);
	};
}

function push(payload: AlertPayload) {
	const severity = payload.severity ?? 'info';
	const duration = payload.duration ?? DEFAULT_DURATION_MS;
	const tag = payload.tag ?? `${payload.title}:${payload.message}`;
	const cooldownMs = payload.cooldownMs ?? DEFAULT_COOLDOWN_MS;

	if (shouldSuppress(tag, cooldownMs)) {
		return;
	}

	const normalized = {
		title: payload.title,
		message: payload.message,
		severity,
		duration,
		tag
	};

	if (mobileLike) {
		void showSystemNotification(normalized).then((shown) => {
			if (!shown) {
				enqueueToast(normalized);
			}
		});
		return;
	}

	enqueueToast(normalized);
}

export const alerts = {
	subscribe,
	initialize,
	push,
	dismiss,
	pause,
	resume
};
