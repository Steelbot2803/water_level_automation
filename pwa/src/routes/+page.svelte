<svelte:options runes={true} />

<script lang="ts">
	import { onMount } from 'svelte';
	import { waterSystem, hasCredentials } from '$lib/stores/system.js';
	import { commandLabels } from '$lib/control.js';
	import type {
		WifiConnectionPhase,
		ArduinoMQTTConnectionPhase,
		MQTTConnectionPhase
	} from '$lib/types.js';
	import {
		wifiConnectionIconsMap,
		arduinoMQTTConnectionIconsMap,
		mqttConnectionIconsMap
	} from '$lib/types.js';
	import { theme } from '$lib/stores/theme.js';
	import { CircleX, Hourglass, LoaderCircle, Menu, RotateCcw } from 'lucide-svelte';
	import SwipeToggle from '$lib/components/SwipeToggle.svelte';
	import type { Option } from '$lib/components/SwipeToggle.svelte';
	import TankCard from '$lib/components/TankCard.svelte';
	import PumpCard from '$lib/components/PumpCard.svelte';
	import MenuDrawer from '$lib/components/Menu.svelte';
	import LoginScreen from '$lib/components/LoginScreen.svelte';
	import SplashScreen from '$lib/components/SplashScreen.svelte';

	// ── Splash visibility ─────────────────────────────────────────────────────
	//
	// Two conditions can dismiss the splash:
	//   1. The store sets initialized = true  (normal path, happens in <1 frame)
	//   2. The 5 s timeout fires              (crash/hang fallback)
	//
	// $state (writable) so both the $effect and the setTimeout can assign to it.
	// $derived is read-only — assigning to it throws a runtime error in Svelte 5.

	const MIN_SPLASH_TIME = 500; // ms
	const MAX_SPLASH_TIME = 10000; // ms

	let splashVisible = $state(true);
	let modeValue: 'auto' | 'manual' = $state('auto');
	let pumpValue: 'borewell' | 'sump' = $state('borewell');
	let overrideSending = $state(false);
	let menuOpen = $state(false);
	let ignoreDeviceSyncUntil = $state(0);

	const showLogin = $derived(!$hasCredentials);
	const eStopped = $derived($waterSystem.device?.alarms?.emergencyStop ?? false);
	const borewellStatus = $derived($waterSystem.device?.motors?.borewell?.status);
	const sumpStatus = $derived($waterSystem.device?.motors?.sump?.status);
	const isConnected = $derived($waterSystem.mqttConnection.mqttPhase === 'connected');
	const mqttConnectionPhase = $derived($waterSystem.mqttConnection.mqttPhase);
	const wifiConnectionPhase = $derived($waterSystem.wifiConnection.wifiPhase);
	const arduinoMQTTConnectionPhase = $derived($waterSystem.arduinoMQTTConnection.mqttPhase);

	const wifiIcon = $derived(wifiConnectionIconsMap[wifiConnectionPhase]);
	const mqttIcon = $derived(mqttConnectionIconsMap[mqttConnectionPhase]);
	const arduinoMqttIcon = $derived(arduinoMQTTConnectionIconsMap[arduinoMQTTConnectionPhase]);

	const WifiIconConfig = $derived(
		wifiIcon === LoaderCircle
			? { component: wifiIcon, class: 'animate-spin' }
			: wifiIcon === Hourglass
				? { component: wifiIcon, class: 'animate-hourglass' }
				: { component: wifiIcon, class: '' }
	);
	const MqttIconConfig = $derived(
		mqttIcon === LoaderCircle
			? { component: mqttIcon, class: 'animate-spin' }
			: mqttIcon === Hourglass
				? { component: mqttIcon, class: 'animate-hourglass' }
				: { component: mqttIcon, class: '' }
	);
	const ArduinoMqttIconConfig = $derived(
		arduinoMqttIcon === LoaderCircle
			? { component: arduinoMqttIcon, class: 'animate-spin' }
			: arduinoMqttIcon === Hourglass
				? { component: arduinoMqttIcon, class: 'animate-hourglass' }
				: { component: arduinoMqttIcon, class: '' }
	);

	const modeOptions: Option<'auto' | 'manual'>[] = [
		{ label: 'Auto', value: 'auto' },
		{ label: 'Manual', value: 'manual' }
	];
	const pumpOptions: Option<'borewell' | 'sump'>[] = [
		{ label: 'Borewell', value: 'borewell' },
		{ label: 'Sump', value: 'sump' }
	];
	const appBadges = [
		{
			label: 'Netlify',
			href: 'https://www.netlify.com/',
			tone: 'border-emerald-200 bg-emerald-50 text-emerald-800'
		},
		{
			label: 'Svelte',
			href: 'https://svelte.dev/',
			tone: 'border-orange-200 bg-orange-50 text-orange-800'
		},
		{
			label: 'HiveMQ',
			href: 'https://www.hivemq.com/',
			tone: 'border-fuchsia-200 bg-fuchsia-50 text-fuchsia-800'
		},
		{
			label: 'Codex',
			href: 'https://chatgpt.com/codex',
			tone: 'border-sky-200 bg-sky-50 text-sky-800'
		},
		{
			label: 'Claude',
			href: 'https://www.anthropic.com/claude',
			tone: 'border-amber-200 bg-amber-50 text-amber-800'
		}
	] as const;

	// ── Phase → tone mapping ──────────────────────────────────────────────────
	//
	// The three phase union types share string values like 'connected' and
	// 'disconnected', so TypeScript cannot safely index a Record<Phase, T> —
	// it treats the union as potentially unresolvable at the call site.
	// A Map<string, T> sidesteps this: lookup is by runtime string value and
	// the function falls back to 'neutral' for any key not in the map.

	type StatusTone = 'neutral' | 'loading' | 'success' | 'error' | 'warning' | 'offline';

	type Phase = WifiConnectionPhase | ArduinoMQTTConnectionPhase | MQTTConnectionPhase;

	const phaseToneMap = new Map<string, StatusTone>([
		['idle', 'neutral'],
		['unknown', 'neutral'],
		['connecting', 'loading'],
		['reconnecting', 'warning'],
		['connected', 'success'],
		['disconnected', 'error'],
		['connection_lost', 'error'],
		['connect_failed', 'error'],
		['ssid_unavailable', 'error'],
		['no_module', 'error'],
		['error', 'error'],
		['offline', 'offline']
	]);

	const badgeToneStyles: Record<StatusTone, string> = {
		neutral: 'border-slate-200 bg-white/96 text-slate-800',
		loading: 'border-amber-200 bg-amber-50/96 text-amber-800',
		success: 'border-emerald-200 bg-emerald-50/96 text-emerald-800',
		error: 'border-rose-200 bg-rose-50/96 text-rose-800',
		warning: 'border-orange-200 bg-orange-50/96 text-orange-800',
		offline: 'border-slate-300 bg-slate-100/96 text-slate-700'
	};

	const shellToneStyles: Record<StatusTone, string> = {
		neutral: 'border-slate-200 bg-white/92',
		loading: 'border-amber-200 bg-amber-50/92',
		success: 'border-emerald-200 bg-emerald-50/92',
		error: 'border-rose-200 bg-rose-50/92',
		warning: 'border-orange-200 bg-orange-50/92',
		offline: 'border-slate-300 bg-slate-100/92'
	};

	// Pre-computed so it doesn't reformat on every MQTT tick.
	const lastUpdateLabel = $derived(
		$waterSystem.device?.receivedAt
			? new Intl.DateTimeFormat(undefined, {
					dateStyle: 'medium',
					timeStyle: 'short'
				}).format($waterSystem.device.receivedAt)
			: null
	);

	// Debounce device → UI sync so rapid MQTT messages don't thrash reactive
	// state and trigger unnecessary layout passes on mobile.
	let syncTimer: ReturnType<typeof setTimeout> | null = null;
	$effect(() => {
		const device = $waterSystem.device;
		if (!device) return;
		if (Date.now() < ignoreDeviceSyncUntil) return;

		if (syncTimer !== null) clearTimeout(syncTimer);
		syncTimer = setTimeout(() => {
			syncTimer = null;
			modeValue = device.mode === 'manual' ? 'manual' : 'auto';
			pumpValue =
				device.mode === 'manual'
					? device.manual_target === 'sump'
						? 'sump'
						: 'borewell'
					: device.auto_prefer_sump
						? 'sump'
						: 'borewell';
		}, 300);
	});

	const modePillColors: Record<'auto' | 'manual', string> = {
		auto: 'bg-emerald-50 shadow-sm border border-emerald-200',
		manual: 'bg-rose-50 shadow-sm border border-rose-200'
	};
	const modeTextColors: Record<'auto' | 'manual', string> = {
		auto: 'text-emerald-700',
		manual: 'text-rose-700'
	};
	const pumpPillColors: Record<'borewell' | 'sump', string> = {
		borewell: 'bg-cyan-50 shadow-sm border border-cyan-200',
		sump: 'bg-violet-50 shadow-sm border border-violet-200'
	};
	const pumpTextColors: Record<'borewell' | 'sump', string> = {
		borewell: 'text-cyan-700',
		sump: 'text-violet-700'
	};

	function getConnectionTone(phase: Phase, type: 'badge' | 'shell'): string {
		const tone = phaseToneMap.get(phase) ?? 'neutral';
		return type === 'badge' ? badgeToneStyles[tone] : shellToneStyles[tone];
	}

	function handleModeChange(value: 'auto' | 'manual', _index: number) {
		ignoreDeviceSyncUntil = Date.now() + 1500;
		waterSystem.sendCommand(value);
		if (value === 'manual')
			waterSystem.sendCommand(`motor ${pumpValue}` as 'motor borewell' | 'motor sump');
		else waterSystem.sendCommand(pumpValue);
		modeValue = value;
	}

	function handlePumpChange(value: 'borewell' | 'sump', _index: number) {
		ignoreDeviceSyncUntil = Date.now() + 1500;
		pumpValue = value;
		if (modeValue === 'manual')
			waterSystem.sendCommand(`motor ${value}` as 'motor borewell' | 'motor sump');
		else waterSystem.sendCommand(value);
	}

	function handleOverride() {
		overrideSending = true;
		waterSystem.sendCommand('override');
		setTimeout(() => {
			overrideSending = false;
		}, 800);
	}

	function eStop(active: boolean) {
		waterSystem.sendCommand(active ? 'estop' : 'resume');
	}

	function unlockPump(pump: 'borewell' | 'sump') {
		waterSystem.sendCommand(pump === 'borewell' ? 'unlock borewell' : 'unlock sump');
	}

	async function showSplash() {
		let startTime = Date.now();
		const elapsed = Date.now() - startTime;
		const remaining = Math.max(0, MIN_SPLASH_TIME - elapsed);

		setTimeout(() => {
			splashVisible = false;
		}, remaining);
	}

	onMount(() => {
		waterSystem.initialize();
		const cleanupTheme = theme.initialize();

		// Fallback: if the store hasn't initialized within 5 s (crash / hang),
		// dismiss the splash anyway so the user sees the app state (even if it's
		// an error phase) rather than being stuck on the splash forever.

		const splashMinTime = setTimeout(() => {
			if (splashVisible) {
				showSplash();
			}
		}, MIN_SPLASH_TIME);

		const splashTimeout = setTimeout(() => {
			if (splashVisible) {
				console.warn('[neptune] splash timeout — forcing dismiss');
				splashVisible = false;
			}
		}, MAX_SPLASH_TIME);

		return () => {
			clearTimeout(splashTimeout);
			clearTimeout(splashMinTime);
			cleanupTheme?.();
		};
	});
</script>

<svelte:head>
	<title>Neptune</title>
	<meta
		name="description"
		content="Mobile control dashboard for the Neptune Water Flow automation system."
	/>
	<meta name="theme-color" content="#0f766e" />
</svelte:head>

<!-- Splash covers the app until the store has resolved credentials from localStorage -->
<SplashScreen visible={splashVisible} />

<div class:overflow-hidden={splashVisible}>
	{#if showLogin}
		<LoginScreen />
	{:else}
		<div
			class="bg-gradient-to-b from-cyan-50 via-white to-slate-100 text-slate-950"
			class:min-h-dvh={!splashVisible}
			class:h-dvh={splashVisible}
		>
			<div class="mx-auto flex max-w-5xl flex-col gap-4 px-4 pt-4 pb-4 sm:px-6">
				<header
					class={`overflow-hidden rounded-[2rem] border p-5 shadow-sm backdrop-blur-sm ${getConnectionTone(mqttConnectionPhase, 'shell')}`}
				>
					<div class="flex flex-col">
						<div class="flex max-w-full items-center justify-between gap-3">
							<p class="text-[0.65rem] font-semibold tracking-[0.28em] text-cyan-900 uppercase">
								Water Flow Automation
							</p>
							<div class="flex items-center gap-1.5">
								<span
									class={`rounded-full p-1 text-xs font-semibold ${getConnectionTone(wifiConnectionPhase, 'badge')}`}
								>
									<WifiIconConfig.component size={20} class={WifiIconConfig.class} />
								</span>
								<span
									class={`rounded-full p-1 text-xs font-semibold ${getConnectionTone(arduinoMQTTConnectionPhase, 'badge')}`}
								>
									<ArduinoMqttIconConfig.component size={20} class={ArduinoMqttIconConfig.class} />
								</span>
								<span
									class={`rounded-full p-1 text-xs font-semibold ${getConnectionTone(mqttConnectionPhase, 'badge')}`}
								>
									<MqttIconConfig.component size={20} class={MqttIconConfig.class} />
								</span>
								<button
									onclick={() => (menuOpen = true)}
									class="ml-1 flex h-11 w-11 items-center justify-center rounded-full border border-white/80 bg-white/70 text-cyan-900/70 shadow-sm transition hover:bg-white hover:text-cyan-900 active:scale-[0.98]"
									aria-label="Open settings"
								>
									<Menu size={18} />
								</button>
							</div>
						</div>
						<div class="mt-4 flex max-w-full items-center justify-between gap-3">
							<h1 class="text-3xl font-semibold tracking-tight uppercase sm:text-4xl">Neptune</h1>
							{#if $waterSystem.device}
								{#if eStopped}
									<button
										class="text-l inline-flex items-center justify-center gap-2 rounded-full border border-emerald-300 bg-emerald-50 px-4 py-2 font-semibold tracking-[0.14em] text-emerald-700 uppercase shadow-sm transition hover:bg-emerald-100 active:scale-[0.98]"
										onclick={() => eStop(false)}
									>
										<RotateCcw size={24} />
										{commandLabels['resume']}
									</button>
								{:else if (borewellStatus && borewellStatus !== 'stopped') || (sumpStatus && sumpStatus !== 'stopped')}
									<button
										class="text-l inline-flex items-center justify-center gap-2 rounded-full border border-rose-300 bg-rose-50 px-4 py-2 font-semibold tracking-[0.14em] text-rose-700 uppercase shadow-sm transition hover:bg-rose-100 active:scale-[0.98]"
										onclick={() => eStop(true)}
									>
										<CircleX size={24} />
										{commandLabels['estop']}
									</button>
								{/if}
							{/if}
						</div>
					</div>
				</header>

				<main class="grid gap-4">
					<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
						<div class="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
							<h2 class="text-xl font-semibold uppercase">Overview</h2>
							{#if lastUpdateLabel}
								<p class="text-xs text-slate-500">Last update {lastUpdateLabel}</p>
							{/if}
						</div>

						{#if !$waterSystem.telemetryReady}
							<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
								Tanks
							</p>
							<div class="mt-2 grid grid-cols-2 gap-3">
								<TankCard variant="overhead" level={$waterSystem.device?.overhead} />
								<TankCard variant="sump" level={$waterSystem.device?.sump} />
							</div>

							<div class="mt-6 h-1 rounded-full bg-slate-200"></div>

							<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
								Pumps
							</p>
							<div class="mt-2 grid grid-cols-2 gap-3">
								<PumpCard
									label="Borewell"
									status={$waterSystem.device?.motors?.borewell?.status}
									onUnlock={() => unlockPump('borewell')}
									unlockDisabled={!isConnected}
								/>
								<PumpCard
									label="Sump"
									status={$waterSystem.device?.motors?.sump?.status}
									onUnlock={() => unlockPump('sump')}
									unlockDisabled={!isConnected}
								/>
							</div>
						{:else}
							<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
								Tanks
							</p>
							<div class="mt-2 grid grid-cols-2 gap-3">
								<div class="relative inline-block">
									<TankCard variant="overhead" level={$waterSystem.device?.overhead} />
								</div>
								<div class="relative inline-block">
									<TankCard variant="sump" level={$waterSystem.device?.sump} />
								</div>
							</div>

							<div class="mt-6 h-1 rounded-full bg-slate-200"></div>

							<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
								Pumps
							</p>
							<div class="mt-2 grid grid-cols-2 gap-3">
								<div class="relative inline-block">
									<PumpCard
										label="Borewell"
										status={$waterSystem.device?.motors?.borewell?.status}
										onUnlock={() => unlockPump('borewell')}
										unlockDisabled={!isConnected}
									/>
								</div>
								<div class="relative inline-block">
									<PumpCard
										label="Sump"
										status={$waterSystem.device?.motors?.sump?.status}
										onUnlock={() => unlockPump('sump')}
										unlockDisabled={!isConnected}
									/>
								</div>
							</div>
						{/if}
					</section>

					<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
						<h2 class="text-xl font-semibold uppercase">Commands</h2>
						<div class="mt-4 grid grid-cols-2 gap-3 sm:grid-cols-3">
							<div
								class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
							>
								<p class="mb-3 text-base font-semibold tracking-[0.15em] text-slate-500 uppercase">
									Mode
								</p>
								<SwipeToggle
									options={modeOptions}
									value={modeValue}
									color={modePillColors[modeValue]}
									textColor={modeTextColors[modeValue]}
									disabled={!isConnected}
									change={handleModeChange}
								/>
							</div>
							<div
								class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
							>
								<p class="mb-3 text-base font-semibold tracking-[0.15em] text-slate-500 uppercase">
									Pump
								</p>
								<SwipeToggle
									options={pumpOptions}
									value={pumpValue}
									color={pumpPillColors[pumpValue]}
									textColor={pumpTextColors[pumpValue]}
									disabled={!isConnected}
									change={handlePumpChange}
								/>
							</div>
							<div
								class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
							>
								<p class="mb-3 text-base font-semibold tracking-[0.15em] text-slate-500 uppercase">
									Override
								</p>
								<button
									aria-label="Override"
									class={`text-l flex min-h-11 w-full items-center justify-center rounded-full border p-3 text-center font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985] disabled:pointer-events-none disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-100 disabled:text-slate-400 disabled:shadow-none ${
										($waterSystem.device?.override ?? false)
											? 'border-orange-300 bg-orange-50 text-orange-700'
											: overrideSending
												? 'animate-pulse border-amber-400 bg-amber-50 text-amber-600'
												: 'border-teal-200 bg-teal-50 text-teal-700 hover:bg-teal-100'
									}`}
									disabled={!isConnected || ($waterSystem.device?.override ?? false)}
									onclick={handleOverride}
								>
									{($waterSystem.device?.override ?? false)
										? 'Override Active'
										: 'Activate Override'}
								</button>
							</div>
						</div>
					</section>
				</main>

				<footer class="flex flex-wrap items-center justify-center gap-2 px-2 pt-1">
					{#each appBadges as badge}
						<a
							href={badge.href}
							target="_blank"
							rel="noreferrer"
							class={`flex items-center justify-center rounded-full border px-2 py-1 text-[0.68rem] font-semibold tracking-[0.18em] uppercase shadow-sm transition hover:-translate-y-0.5 hover:shadow active:scale-[0.98] ${badge.tone}`}
						>
							{badge.label}
						</a>
					{/each}
				</footer>

				<MenuDrawer bind:open={menuOpen} />
			</div>
		</div>
	{/if}
</div>
