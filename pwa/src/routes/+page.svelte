<script lang="ts">
	import { onMount } from 'svelte';
	import { pumpPreference } from '$lib/stores/pump.js';
	import { waterSystem } from '$lib/stores/system.js';
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
	import { Hourglass, LoaderCircle, Menu, OctagonX, RotateCcw } from 'lucide-svelte';
	import SwipeToggle from '$lib/components/SwipeToggle.svelte';
	import type { Option } from '$lib/components/SwipeToggle.svelte';
	import TankCard from '$lib/components/TankCard.svelte';
	import PumpCard from '$lib/components/PumpCard.svelte';
	import MenuDrawer from '$lib/components/Menu.svelte';

	let modeValue: 'auto' | 'manual' = $state('auto');
	let pumpValue: 'borewell' | 'sump' = $state('borewell');
	let overrideSending = $state(false);
	let menuOpen = $state(false);
	const eStopped = $derived($waterSystem.device?.alarms?.emergencyStop ?? false);
	const borewellStatus = $derived($waterSystem.device?.motors?.borewell?.status);
	const sumpStatus = $derived($waterSystem.device?.motors?.sump?.status);

	const wifiIcon = $derived(wifiConnectionIconsMap[$waterSystem.wifiConnection.wifiPhase]);
	const mqttIcon = $derived(mqttConnectionIconsMap[$waterSystem.mqttConnection.mqttPhase]);
	const arduinoMqttIcon = $derived(
		arduinoMQTTConnectionIconsMap[$waterSystem.arduinoMQTTConnection.mqttPhase]
	);

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

	const isConnected = $derived($waterSystem.mqttConnection.mqttPhase === 'connected');

	const wifiConnectionBadges: Record<WifiConnectionPhase, string> = {
		unknown: 'bg-slate-200 text-slate-700',
		connecting: 'bg-amber-100 text-amber-900',
		connected: 'bg-emerald-100 text-emerald-900',
		disconnected: 'bg-rose-200 text-rose-950',
		connection_lost: 'bg-rose-200 text-rose-950',
		connect_failed: 'bg-rose-200 text-rose-950',
		ssid_unavailable: 'bg-rose-200 text-rose-950',
		no_module: 'bg-rose-200 text-rose-950'
	};

	const arduinoMQTTConnectionBadges: Record<ArduinoMQTTConnectionPhase, string> = {
		unknown: 'bg-slate-200 text-slate-700',
		connected: 'bg-emerald-100 text-emerald-900',
		disconnected: 'bg-rose-200 text-rose-950'
	};

	const mqttConnectionBadges: Record<MQTTConnectionPhase, string> = {
		idle: 'bg-slate-200 text-slate-700',
		connecting: 'bg-amber-100 text-amber-900',
		connected: 'bg-emerald-100 text-emerald-900',
		reconnecting: 'bg-orange-100 text-orange-900',
		offline: 'bg-slate-300 text-slate-800',
		error: 'bg-rose-200 text-rose-950'
	};

	const connectionShells: Record<MQTTConnectionPhase, string> = {
		idle: 'border-slate-200 bg-white/92',
		connecting: 'border-amber-200 bg-amber-50/92',
		connected: 'border-emerald-200 bg-emerald-50/92',
		reconnecting: 'border-orange-200 bg-orange-50/92',
		offline: 'border-slate-300 bg-slate-100/92',
		error: 'border-rose-200 bg-rose-50/92'
	};

	onMount(() => {
		waterSystem.initialize();
		const cleanupTheme = theme.initialize();
		const unsubDevice = waterSystem.subscribe((s) => {
			if (!s.device) return;
			modeValue = s.device.mode === 'manual' ? 'manual' : 'auto';
			pumpValue = s.device.auto_prefer_sump ? 'sump' : 'borewell';
			unsubDevice();
		});
		return () => {
			cleanupTheme?.();
			unsubDevice();
		};
	});

	function formatTimestamp(value?: number, compact = false) {
		if (!value) return 'Not available';

		return new Intl.DateTimeFormat(undefined, {
			...(compact
				? { hour: 'numeric', minute: '2-digit' }
				: { dateStyle: 'medium', timeStyle: 'short' })
		}).format(value);
	}

	const modeColors: Record<'auto' | 'manual', string> = {
		auto: 'bg-emerald-400',
		manual: 'bg-red-400'
	};

	const pumpColors: Record<'borewell' | 'sump', string> = {
		borewell: 'bg-teal-500',
		sump: 'bg-lime-600'
	};

	function handleModeChange(value: 'auto' | 'manual', _index: number) {
		waterSystem.sendCommand(value);
		if (value === 'manual') {
			waterSystem.sendCommand(`motor ${pumpValue}` as 'motor borewell' | 'motor sump');
		}
		modeValue = value;
	}

	function handlePumpChange(value: 'borewell' | 'sump', _index: number) {
		pumpPreference.set(value);
		pumpValue = value;
		if (modeValue === 'manual') {
			waterSystem.sendCommand(`motor ${value}` as 'motor borewell' | 'motor sump');
		} else {
			waterSystem.sendCommand(value);
		}
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
</script>

<svelte:head>
	<title>Neptune</title>
	<meta
		name="description"
		content="Mobile control dashboard for the Neptune water level automation system."
	/>
	<meta name="theme-color" content="#0f766e" />
</svelte:head>

<div class="min-h-dvh bg-gradient-to-b from-cyan-50 via-white to-slate-100 text-slate-950">
	<div
		class="mx-auto flex max-w-5xl flex-col gap-4 px-4 pt-[calc(1rem+env(safe-area-inset-top))] pb-[calc(1rem+env(safe-area-inset-bottom))] sm:px-6"
	>
		<header
			class={`overflow-hidden rounded-[2rem] border p-5 shadow-sm backdrop-blur-sm ${connectionShells[$waterSystem.mqttConnection.mqttPhase]}`}
		>
			<div class="flex flex-col">
				<div class="flex max-w-full items-center justify-between gap-3">
					<p class="text-[0.65rem] font-semibold tracking-[0.28em] text-cyan-900 uppercase">
						Water Level Automation
					</p>
					<!-- Status badges + menu button, replacing the old theme pill -->
					<div class="flex items-center gap-1.5">
						<span
							class={`rounded-full p-1 text-xs font-semibold ${wifiConnectionBadges[$waterSystem.wifiConnection.wifiPhase]}`}
						>
							<WifiIconConfig.component size={20} class={WifiIconConfig.class} />
						</span>
						<span
							class={`rounded-full p-1 text-xs font-semibold ${arduinoMQTTConnectionBadges[$waterSystem.arduinoMQTTConnection.mqttPhase]}`}
						>
							<ArduinoMqttIconConfig.component size={20} class={ArduinoMqttIconConfig.class} />
						</span>
						<span
							class={`rounded-full p-1 text-xs font-semibold ${mqttConnectionBadges[$waterSystem.mqttConnection.mqttPhase]}`}
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
								<OctagonX size={24} class="translate-y-0.25" />
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
					{#if $waterSystem.device}
						<p class="text-xs text-slate-500">
							Last update {formatTimestamp($waterSystem.device.receivedAt)}
						</p>
					{/if}
				</div>

				<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">Tanks</p>
				<div class="mt-2 grid gap-3 sm:grid-cols-2">
					<TankCard variant="overhead" level={$waterSystem.device?.overhead} />
					<TankCard variant="sump" level={$waterSystem.device?.sump} />
				</div>

				<div class="mt-6 h-1 rounded-full bg-slate-200"></div>

				<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">Pumps</p>
				<div class="mt-2 grid gap-3 sm:grid-cols-2">
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
			</section>

			<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
					<div>
						<h2 class="text-xl font-semibold uppercase">Commands</h2>
					</div>
				</div>

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
							color={modeColors[modeValue]}
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
							color={pumpColors[pumpValue]}
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
										? 'text-amber-060 animate-pulse border-amber-400 bg-amber-50'
										: 'border-teal-200 bg-teal-50 text-teal-700 hover:bg-teal-100'
							}`}
							disabled={!isConnected || ($waterSystem.device?.override ?? false)}
							onclick={handleOverride}
						>
							{($waterSystem.device?.override ?? false) ? 'Override Active' : 'Activate Override'}
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
