/// <reference lib="webworker" />

declare const self: ServiceWorkerGlobalScope;

const CACHE = 'neptune-v1';

const PRECACHE = ['/', '/manifest.json'];

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
	// Only cache GET requests, let MQTT websocket traffic through untouched
	if (event.request.method !== 'GET') return;

	event.respondWith(caches.match(event.request).then((cached) => cached ?? fetch(event.request)));
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
