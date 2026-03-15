/// <reference lib="webworker" />

declare const self: ServiceWorkerGlobalScope;

self.addEventListener('install', () => {
	void self.skipWaiting();
});

self.addEventListener('activate', (event) => {
	event.waitUntil(self.clients.claim());
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
				if ('focus' in client) {
					return client.focus();
				}
			}

			return self.clients.openWindow('/');
		})()
	);
});

export {};
