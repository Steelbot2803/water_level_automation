/// <reference lib="webworker" />

declare const self: ServiceWorkerGlobalScope;

const CACHE_VERSION = 'v3';
const CACHE = `neptune-${CACHE_VERSION}`;
const OFFLINE_NAV_FALLBACK = '/';

// Keep a minimal offline document fallback so navigations can recover when
// preload/network fail. We still prefer fresh network HTML for navigations.
const PRECACHE = ['/manifest.json', OFFLINE_NAV_FALLBACK];
const STATIC_ASSET_RE = /\.(js|css|png|svg|ico|woff2?)(\?|$)/;

self.addEventListener('install', (event) => {
	event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(PRECACHE)));
	void self.skipWaiting();
});

self.addEventListener('activate', (event) => {
	event.waitUntil(
		(async () => {
			const keys = await caches.keys();
			await Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k)));
			if ('navigationPreload' in self.registration) {
				await self.registration.navigationPreload.enable();
			}
			await self.clients.claim();
		})()
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
	if (request.mode === 'navigate') {
		event.respondWith(
			(async () => {
				const cache = await caches.open(CACHE);
				try {
					const preload = await event.preloadResponse;
					if (preload) {
						if (preload.ok) {
							void cache.put(OFFLINE_NAV_FALLBACK, preload.clone());
						}
						return preload;
					}
					const network = await fetch(request);
					if (network.ok) {
						void cache.put(OFFLINE_NAV_FALLBACK, network.clone());
					}
					return network;
				} catch {
					const fallback = await cache.match(OFFLINE_NAV_FALLBACK);
					if (fallback) return fallback;
					return Response.error();
				}
			})()
		);
		return;
	}

	if (!STATIC_ASSET_RE.test(url.pathname)) return;

	// Stale-while-revalidate for static assets: fast first paint with background refresh.
	event.respondWith(
		(async () => {
			const cache = await caches.open(CACHE);
			const cached = await cache.match(request);
			const fetchAndUpdate = fetch(request)
				.then((response) => {
					if (response.ok) {
						void cache.put(request, response.clone());
					}
					return response;
				})
				.catch(() => null);

			if (cached) {
				void fetchAndUpdate;
				return cached;
			}

			const network = await fetchAndUpdate;
			if (network) return network;
			return Response.error();
		})()
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
