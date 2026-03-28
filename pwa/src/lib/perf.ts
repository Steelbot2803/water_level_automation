import { browser } from '$app/environment';

type CounterName =
	| 'mqtt_messages'
	| 'mqtt_parse_ms'
	| 'store_updates'
	| 'store_update_ms'
	| 'ui_apply_dropped'
	| 'ui_apply_frames'
	| 'frame_miss';

const ENABLED = browser && import.meta.env.PUBLIC_PERF_DEBUG === 'true';
const REPORT_INTERVAL_MS = 10000;

const counters: Record<CounterName, number> = {
	mqtt_messages: 0,
	mqtt_parse_ms: 0,
	store_updates: 0,
	store_update_ms: 0,
	ui_apply_dropped: 0,
	ui_apply_frames: 0,
	frame_miss: 0
};

let started = false;
let reportTimer: ReturnType<typeof setInterval> | null = null;
let frameRafId: number | null = null;
let lastFrameAt = 0;

function report() {
	if (!ENABLED) return;
	const avgParse = counters.mqtt_messages ? counters.mqtt_parse_ms / counters.mqtt_messages : 0;
	const avgStoreUpdate = counters.store_updates
		? counters.store_update_ms / counters.store_updates
		: 0;

	console.table({
		mqttMessages: counters.mqtt_messages,
		avgParseMs: Number(avgParse.toFixed(2)),
		storeUpdates: counters.store_updates,
		avgStoreUpdateMs: Number(avgStoreUpdate.toFixed(2)),
		uiApplyFrames: counters.ui_apply_frames,
		uiApplyDropped: counters.ui_apply_dropped,
		frameMisses: counters.frame_miss
	});
}

function frameProbe(now: number) {
	if (lastFrameAt && now - lastFrameAt > 20) {
		counters.frame_miss += 1;
	}
	lastFrameAt = now;
	frameRafId = requestAnimationFrame(frameProbe);
}

export const perf = {
	enabled: ENABLED,
	start() {
		if (!ENABLED || started) return;
		started = true;
		reportTimer = setInterval(report, REPORT_INTERVAL_MS);
		frameRafId = requestAnimationFrame(frameProbe);
	},
	stop() {
		if (!ENABLED) return;
		if (reportTimer) clearInterval(reportTimer);
		if (frameRafId !== null) cancelAnimationFrame(frameRafId);
		reportTimer = null;
		frameRafId = null;
		started = false;
	},
	add(name: CounterName, amount = 1) {
		if (!ENABLED) return;
		counters[name] += amount;
	},
	time<T>(fn: () => T, counter: CounterName): T {
		if (!ENABLED) return fn();
		const startedAt = performance.now();
		const result = fn();
		counters[counter] += performance.now() - startedAt;
		return result;
	}
};
