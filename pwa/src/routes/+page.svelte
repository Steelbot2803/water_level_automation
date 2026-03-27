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

	const MIN_SPLASH_TIME = 3000; // ms
	const MAX_SPLASH_TIME = 100000; // ms

	let splashVisible = $state(true);
	let splashDone = $state(false); // trails splashVisible by the fade-out duration
	let modeValue: 'auto' | 'manual' = $state('auto');
	let pumpValue: 'borewell' | 'sump' = $state('borewell');
	let overrideSending = $state(false);
	let menuOpen = $state(false);
	let ignoreDeviceSyncUntil = $state(0);
	let openBadge = $state<string | null>(null);

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

	// Use inline color values to avoid Tailwind purge issues on dynamic classes.
	const wifiConnectionBadges: Record<WifiConnectionPhase, string> = {
		unknown: 'background:#e2e8f0; color:#334155;',
		connecting: 'background:#fef3c7; color:#422006;',
		connected: 'background:#d1fae5; color:#064e3b;',
		disconnected: 'background:#fecdd3; color:#4c0519;',
		connection_lost: 'background:#fecdd3; color:#4c0519;',
		connect_failed: 'background:#fecdd3; color:#4c0519;',
		ssid_unavailable: 'background:#fecdd3; color:#4c0519;',
		no_module: 'background:#fecdd3; color:#4c0519;'
	};

	const arduinoMQTTConnectionBadges: Record<ArduinoMQTTConnectionPhase, string> = {
		unknown: 'background:#e2e8f0; color:#334155;',
		connected: 'background:#d1fae5; color:#064e3b;',
		disconnected: 'background:#fecdd3; color:#4c0519;'
	};

	const mqttConnectionBadges: Record<MQTTConnectionPhase, string> = {
		idle: 'background:#e2e8f0; color:#334155;',
		connecting: 'background:#fef3c7; color:#422006;',
		connected: 'background:#d1fae5; color:#064e3b;',
		reconnecting: 'background:#ffedd5; color:#431407;',
		offline: 'background:#cbd5e1; color:#1e293b;',
		error: 'background:#fecdd3; color:#4c0519;'
	};

	const connectionShells: Record<MQTTConnectionPhase, string> = {
		idle: 'border:1px solid #e2e8f0; background:rgba(255,255,255,0.92);',
		connecting: 'border:1px solid #fde68a; background:rgba(255,251,235,0.92);',
		connected: 'border:1px solid #6ee7b7; background:rgba(236,253,245,0.92);',
		reconnecting: 'border:1px solid #fed7aa; background:rgba(255,247,237,0.92);',
		offline: 'border:1px solid #cbd5e1; background:rgba(241,245,249,0.92);',
		error: 'border:1px solid #fecdd3; background:rgba(255,241,242,0.92);'
	};

	// Select shell color from the worst active connection state.
	const combinedShellStyle = $derived(
		(() => {
			const allGreen =
				wifiConnectionPhase === 'connected' &&
				arduinoMQTTConnectionPhase === 'connected' &&
				mqttConnectionPhase === 'connected';
			if (allGreen) return connectionShells['connected'];

			const hasError =
				[
					'disconnected',
					'connection_lost',
					'connect_failed',
					'ssid_unavailable',
					'no_module'
				].includes(wifiConnectionPhase) ||
				arduinoMQTTConnectionPhase === 'disconnected' ||
				mqttConnectionPhase === 'error';
			if (hasError) return connectionShells['error'];

			const hasOffline = mqttConnectionPhase === 'offline';
			if (hasOffline) return connectionShells['offline'];

			const hasReconnecting = mqttConnectionPhase === 'reconnecting';
			if (hasReconnecting) return connectionShells['reconnecting'];

			const hasConnecting =
				wifiConnectionPhase === 'connecting' || mqttConnectionPhase === 'connecting';
			if (hasConnecting) return connectionShells['connecting'];

			return connectionShells['idle'];
		})()
	);

	const phaseLabels: Record<string, string> = {
		idle: 'Idle',
		unknown: 'Unknown',
		connecting: 'Connecting',
		reconnecting: 'Reconnecting',
		connected: 'Connected',
		disconnected: 'Disconnected',
		connection_lost: 'Connection lost',
		connect_failed: 'Connection failed',
		ssid_unavailable: 'SSID unavailable',
		no_module: 'No WiFi module',
		error: 'Error',
		offline: 'Offline'
	};

	const badgeConfigs = $derived([
		{
			key: 'wifi',
			phase: wifiConnectionPhase,
			Icon: WifiIconConfig.component,
			iconClass: WifiIconConfig.class,
			label: 'WiFi'
		},
		{
			key: 'arduinoMqtt',
			phase: arduinoMQTTConnectionPhase,
			Icon: ArduinoMqttIconConfig.component,
			iconClass: ArduinoMqttIconConfig.class,
			label: 'Arduino MQTT'
		},
		{
			key: 'broker',
			phase: mqttConnectionPhase,
			Icon: MqttIconConfig.component,
			iconClass: MqttIconConfig.class,
			label: 'Broker'
		}
	]);

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

	function handleModeChange(value: 'auto' | 'manual') {
		ignoreDeviceSyncUntil = Date.now() + 1500;
		waterSystem.sendCommand(value);
		if (value === 'manual')
			waterSystem.sendCommand(`motor ${pumpValue}` as 'motor borewell' | 'motor sump');
		else waterSystem.sendCommand(pumpValue);
		modeValue = value;
	}

	function handlePumpChange(value: 'borewell' | 'sump') {
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

	onMount(() => {
		waterSystem.initialize();
		const cleanupTheme = theme.initialize();

		const handleOutsideClick = () => {
			openBadge = null;
		};
		document.addEventListener('click', handleOutsideClick);

		const splashMinTime = setTimeout(() => {
			splashVisible = false;
			setTimeout(() => {
				splashDone = true;
			}, 300);
		}, MIN_SPLASH_TIME);

		// Hard fallback in case initialization stalls.
		const splashTimeout = setTimeout(() => {
			if (splashVisible) {
				console.warn('[neptune] splash timeout — forcing dismiss');
				splashVisible = false;
				setTimeout(() => {
					splashDone = true;
				}, 300);
			}
		}, MAX_SPLASH_TIME);

		return () => {
			clearTimeout(splashMinTime);
			clearTimeout(splashTimeout);
			cleanupTheme?.();
			document.removeEventListener('click', handleOutsideClick);
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

<div class:overflow-hidden={!splashDone}>
	{#if showLogin}
		<LoginScreen />
	{:else}
		<div
			class="bg-gradient-to-b from-cyan-50 via-white to-slate-100 text-slate-950"
			class:min-h-dvh={splashDone}
			class:h-dvh={!splashDone}
		>
			<div class="mx-auto flex max-w-5xl flex-col gap-4 px-4 pt-4 pb-4 sm:px-6">
				<header
					class="overflow-hidden rounded-[2rem] p-5 shadow-sm backdrop-blur-sm"
					style={combinedShellStyle}
				>
					<div class="flex flex-col">
						<div class="flex max-w-full items-center justify-between gap-3">
							<p class="text-[0.65rem] font-semibold tracking-[0.28em] text-cyan-900 uppercase">
								Water Flow Automation
							</p>
							<div class="flex items-center gap-1.5">
								{#each badgeConfigs as { key, phase, Icon, iconClass, label } (key)}
									<div class="relative">
										<button
											type="button"
											onclick={(e) => {
												e.stopPropagation();
												openBadge = openBadge === key ? null : key;
											}}
											class="flex h-11 w-11 items-center justify-center rounded-full transition active:scale-[0.95]"
											style={key === 'wifi'
												? wifiConnectionBadges[phase as WifiConnectionPhase]
												: key === 'arduinoMqtt'
													? arduinoMQTTConnectionBadges[phase as ArduinoMQTTConnectionPhase]
													: mqttConnectionBadges[phase as MQTTConnectionPhase]}
											aria-label="{label}: {phaseLabels[phase] ?? phase}"
										>
											<Icon size={20} class={iconClass} />
										</button>
										{#if openBadge === key}
											<div
												class="absolute top-full left-1/2 z-50 mt-2 min-w-max -translate-x-1/2 rounded-2xl border border-slate-200 bg-white px-3 py-2 shadow-lg"
												role="tooltip"
											>
												<p class="text-sm font-semibold tracking-[0.15em] text-slate-500 uppercase">
													{label}
												</p>
												<p class="mt-0.5 text-sm font-semibold text-slate-800 capitalize uppercase">
													{phaseLabels[phase] ?? phase}
												</p>
											</div>
										{/if}
									</div>
								{/each}
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
							<h1 class="text-4xl font-semibold tracking-tight uppercase sm:text-5xl">Neptune</h1>
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
					{#each appBadges as badge (badge.label)}
						<button
							type="button"
							onclick={() => window.open(badge.href, '_blank', 'noopener,noreferrer')}
							class={`flex items-center justify-center rounded-full border px-2 py-1 text-[0.68rem] font-semibold tracking-[0.18em] uppercase shadow-sm transition hover:-translate-y-0.5 hover:shadow active:scale-[0.98] ${badge.tone}`}
						>
							{badge.label}
						</button>
					{/each}
				</footer>

				<MenuDrawer bind:open={menuOpen} />
			</div>
		</div>
	{/if}
</div>
