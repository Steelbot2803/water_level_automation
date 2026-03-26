/// <reference lib="webworker" />

declare const self: ServiceWorkerGlobalScope;

const CACHE = 'neptune-v2';

// Only precache truly static assets — never the HTML shell.
// The HTML shell must always come from the network so SvelteKit's
// hydration data is fresh and the MQTT store initialises correctly.
const PRECACHE = ['/manifest.json'];

self.addEventListener('install', (event) => {
	event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(PRECACHE)));
	void self.skipWaiting();
});

self.addEventListener('activate', (event) => {
	event.waitUntil(
		caches
			.keys()
			.then((keys) => Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k))))
			.then(() => self.clients.claim())
	);
});

self.addEventListener('fetch', (event) => {
	const { request } = event;

	if (request.method !== 'GET') return;

	const url = new URL(request.url);

	// Never cache cross-origin requests (HiveMQ WebSocket, CDN fonts, etc.)
	if (url.origin !== self.location.origin) return;

	// Never cache navigation requests — always go to the network so the app
	// shell is fresh and credentials/store state initialise correctly.
	if (request.mode === 'navigate') return;

	// For same-origin static assets (JS, CSS, images): cache-first.
	// SvelteKit content-hashes its bundles so stale hits are impossible.
	event.respondWith(
		caches.match(request).then((cached) => {
			if (cached) return cached;
			return fetch(request).then((response) => {
				// Only cache successful opaque-safe responses for static assets.
				if (response.ok && url.pathname.match(/\.(js|css|png|svg|ico|woff2?)(\?|$)/)) {
					caches.open(CACHE).then((cache) => cache.put(request, response.clone()));
				}
				return response;
			});
		})
	);
});

self.addEventListener('notificationclick', (event) => {
	event.notification.close();
	event.waitUntil(
		(async () => {
			const windowClients = await self.clients.matchAll({
				type: 'window',
				includeUncontrolled: true
			});
			for (const client of windowClients) {
				if ('focus' in client) return client.focus();
			}
			return self.clients.openWindow('/');
		})()
	);
});

export {};
