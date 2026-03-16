<script lang="ts">
	import { onMount } from 'svelte';
	import { pumpPreference } from '../lib/stores/pump.js';
	import { mode } from '../lib/stores/mode.js';
	import { waterSystem } from '../lib/stores/system.js';
	import { commandLabels } from '../lib/control.js';
	import type { ConnectionPhase, OverheadLevel, SumpLevel } from '../lib/types.js';

	let overrideSending = false;

	waterSystem.subscribe((state) => {
		if (state.device) mode.seedFromDevice(state.device.mode);
	});

	$: isConnected = $waterSystem.connection.phase === 'connected';

	const connectionBadges: Record<ConnectionPhase, string> = {
		idle: 'bg-slate-200 text-slate-700',
		connecting: 'bg-amber-100 text-amber-900',
		connected: 'bg-emerald-100 text-emerald-900',
		reconnecting: 'bg-orange-100 text-orange-900',
		offline: 'bg-slate-300 text-slate-800',
		error: 'bg-rose-200 text-rose-950'
	};

	const connectionShells: Record<ConnectionPhase, string> = {
		idle: 'border-slate-200 bg-white/92',
		connecting: 'border-amber-200 bg-amber-50/92',
		connected: 'border-emerald-200 bg-emerald-50/92',
		reconnecting: 'border-orange-200 bg-orange-50/92',
		offline: 'border-slate-300 bg-slate-100/92',
		error: 'border-rose-200 bg-rose-50/92'
	};

	const overheadLabels: Record<OverheadLevel, string> = {
		critical: 'Critical',
		low: 'Low',
		medium: 'Medium',
		high: 'High'
	};

	const sumpLabels: Record<SumpLevel, string> = {
		critical: 'Critical',
		low: 'Low',
		high: 'High'
	};

	const overheadFillHeights: Record<OverheadLevel, string> = {
		critical: '10%',
		low: '35%',
		medium: '65%',
		high: '90%'
	};

	const sumpFillHeights: Record<SumpLevel, string> = {
		critical: '10%',
		low: '50%',
		high: '90%'
	};

	onMount(() => {
		waterSystem.initialize();
	});

	function formatTimestamp(value?: number, compact = false) {
		if (!value) return 'Not available';

		return new Intl.DateTimeFormat(undefined, {
			...(compact
				? { hour: 'numeric', minute: '2-digit' }
				: { dateStyle: 'medium', timeStyle: 'short' })
		}).format(value);
	}

	function overheadTone(level?: OverheadLevel) {
		switch (level) {
			case 'high':
				return 'from-emerald-400 via-emerald-500 to-emerald-700';
			case 'medium':
				return 'from-cyan-400 via-sky-500 to-sky-700';
			case 'low':
				return 'from-amber-300 via-amber-400 to-orange-500';
			case 'critical':
				return 'from-rose-400 via-rose-500 to-rose-700';
			default:
				return 'from-slate-300 to-slate-400';
		}
	}

	function sumpTone(level?: SumpLevel) {
		switch (level) {
			case 'high':
				return 'from-emerald-400 via-teal-500 to-teal-700';
			case 'low':
				return 'from-amber-300 via-yellow-400 to-orange-500';
			case 'critical':
				return 'from-rose-400 via-rose-500 to-rose-700';
			default:
				return 'from-sky-400 via-cyan-500 to-cyan-700';
		}
	}

	function handleOverride() {
		overrideSending = true;
		waterSystem.sendCommand('override');
		setTimeout(() => {
			overrideSending = false;
		}, 800);
	}
</script>

<svelte:head>
	<title>NEPTUNE</title>
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
			class={`overflow-hidden rounded-[2rem] border p-5 shadow-sm backdrop-blur-sm ${connectionShells[$waterSystem.connection.phase]}`}
		>
			<div class="flex flex-col">
				<div class="flex max-w-full items-center justify-between gap-3">
					<p class="text-[0.65rem] font-semibold tracking-[0.28em] text-cyan-900 uppercase">
						Water Level Automation
					</p>
					<span
						class={`rounded-full px-3 py-1 text-xs font-semibold tracking-wide uppercase ${connectionBadges[$waterSystem.connection.phase]}`}
					>
						{$waterSystem.connection.phase}
					</span>
				</div>
				<h1 class="text-3xl font-semibold tracking-tight sm:text-4xl">NEPTUNE</h1>
			</div>
		</header>

		<main class="grid gap-4">
			<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
					<div>
						<h2 class="text-base font-semibold">Tank Levels</h2>
					</div>

					{#if $waterSystem.device}
						<p class="text-xs text-slate-500">
							Last update {formatTimestamp($waterSystem.device.receivedAt)}
						</p>
					{/if}
				</div>

				{#if $waterSystem.device}
					<div class="mt-4 grid gap-3 sm:grid-cols-2">
						<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
							<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
								<div class="relative mx-auto h-56 w-28">
									<div class="absolute inset-x-5 top-0 h-4 rounded-full bg-slate-300/90"></div>
									<div
										class="absolute inset-x-0 top-3 bottom-0 rounded-[2rem] border-[6px] border-slate-300 bg-white shadow-inner"
									>
										<div class="absolute inset-x-3 top-5 bottom-5 flex flex-col justify-between">
											<div class="border-t border-dashed border-slate-300/80"></div>
											<div class="border-t border-dashed border-slate-300/80"></div>
											<div class="border-t border-dashed border-slate-300/80"></div>
											<div class="border-t border-dashed border-slate-300/80"></div>
										</div>
										<div
											class={`absolute inset-x-0 bottom-0 overflow-hidden rounded-b-[1.6rem] bg-gradient-to-t transition-[height] duration-500 ${overheadTone($waterSystem.device.overhead)}`}
											style={`height: ${overheadFillHeights[$waterSystem.device.overhead]};`}
										>
											<div
												class="absolute inset-0 [background-image:linear-gradient(135deg,rgba(255,255,255,0.2)_25%,transparent_25%,transparent_50%,rgba(255,255,255,0.2)_50%,rgba(255,255,255,0.2)_75%,transparent_75%,transparent)] [background-size:24px_24px] opacity-40"
											></div>
											<div
												class="absolute inset-x-4 top-2 h-2 rounded-full bg-white/40 blur-[1px]"
											></div>
										</div>
									</div>
									<div
										class="absolute top-8 -right-14 flex h-44 flex-col justify-between text-[0.65rem] font-semibold tracking-[0.2em] text-slate-400 uppercase"
									>
										<span>High</span>
										<span>Med</span>
										<span>Low</span>
										<span>Crit</span>
									</div>
								</div>

								<div>
									<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
										Overhead Tank
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950">
										{overheadLabels[$waterSystem.device.overhead]}
									</p>
								</div>
							</div>
						</div>

						<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
							<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
								<div class="relative mx-auto h-56 w-28">
									<div
										class="absolute inset-x-4 top-0 h-6 rounded-t-[1.4rem] border-4 border-b-0 border-slate-300 bg-slate-100"
									></div>
									<div
										class="absolute inset-x-0 top-5 bottom-0 rounded-[2rem] border-[6px] border-slate-300 bg-white shadow-inner"
									>
										<div class="absolute inset-x-3 top-5 bottom-5 flex flex-col justify-between">
											<div class="border-t border-dashed border-slate-300/80"></div>
											<div class="border-t border-dashed border-slate-300/80"></div>
											<div class="border-t border-dashed border-slate-300/80"></div>
										</div>
										<div
											class={`absolute inset-x-0 bottom-0 overflow-hidden rounded-b-[1.6rem] bg-gradient-to-t transition-[height] duration-500 ${sumpTone($waterSystem.device.sump)}`}
											style={`height: ${sumpFillHeights[$waterSystem.device.sump]};`}
										>
											<div
												class="absolute inset-0 [background-image:radial-gradient(circle_at_20%_20%,rgba(255,255,255,0.3),transparent_30%),radial-gradient(circle_at_80%_30%,rgba(255,255,255,0.22),transparent_28%)] opacity-35"
											></div>
											<div
												class="absolute inset-x-4 top-2 h-2 rounded-full bg-white/35 blur-[1px]"
											></div>
										</div>
									</div>
									<div
										class="absolute top-10 -right-14 flex h-40 flex-col justify-between text-[0.65rem] font-semibold tracking-[0.2em] text-slate-400 uppercase"
									>
										<span>High</span>
										<span>Low</span>
										<span>Crit</span>
									</div>
								</div>

								<div>
									<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
										Sump Tank
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950">
										{sumpLabels[$waterSystem.device.sump]}
									</p>
								</div>
							</div>
						</div>
						{#if $waterSystem.device?.motors.borewell.status === 'dry_run_lock'}
							<div class="col-span-2 sm:col-span-1">
								<button
									class="min-h-28 w-full rounded-[1.5rem] border border-rose-700/10 bg-rose-600 px-4 py-4 text-left text-white shadow-sm transition active:scale-[0.99] disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-200 disabled:text-slate-500"
									disabled={$waterSystem.connection.phase !== 'connected'}
									onclick={() => waterSystem.sendCommand('unlock borewell')}
								>
									<p class="text-sm font-semibold">{commandLabels['unlock borewell']}</p>
									<p class="mt-1 text-xs opacity-75">Borewell dry-run protection active</p>
								</button>
							</div>
						{/if}

						{#if $waterSystem.device?.motors.sump.status === 'dry_run_lock'}
							<div class="col-span-2 sm:col-span-1">
								<button
									class="min-h-28 w-full rounded-[1.5rem] border border-rose-700/10 bg-rose-600 px-4 py-4 text-left text-white shadow-sm transition active:scale-[0.99] disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-200 disabled:text-slate-500"
									disabled={$waterSystem.connection.phase !== 'connected'}
									onclick={() => waterSystem.sendCommand('unlock sump')}
								>
									<p class="text-sm font-semibold">{commandLabels['unlock sump']}</p>
									<p class="mt-1 text-xs opacity-75">Sump dry-run protection active</p>
								</button>
							</div>
						{/if}
					</div>
				{:else}
					<div class="mt-4 rounded-[1.5rem] bg-stone-100 p-5 text-sm leading-6 text-slate-600">
						<p class="font-semibold text-slate-800">No telemetry yet.</p>
						<p class="mt-2">
							Once the broker connection is healthy, the controller should publish its retained
							status and this screen will populate automatically.
						</p>
					</div>
				{/if}
			</section>

			<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
					<div>
						<h2 class="text-base font-semibold">Commands</h2>
					</div>
					<span
						class={`rounded-full px-3 py-1 text-xs font-semibold tracking-wide uppercase ${$waterSystem.connection.phase === 'connected' ? 'bg-emerald-100 text-emerald-900' : 'bg-slate-200 text-slate-700'}`}
					>
						{$waterSystem.connection.phase === 'connected'
							? 'Publish Enabled'
							: 'Connect to Enable'}
					</span>
				</div>

				<div class="mt-4 grid grid-cols-2 gap-3 sm:grid-cols-3">
					<div
						class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
					>
						<p class="mb-3 text-xs font-semibold tracking-[0.15em] text-slate-500 uppercase">
							Mode
						</p>
						<div class="relative flex h-11 overflow-hidden rounded-2xl bg-slate-200 p-1">
							<div
								class={`absolute top-1 bottom-1 rounded-xl shadow-sm transition-all duration-200 ${
									$mode !== 'manual'
										? 'right-[calc(50%+2px)] left-1 bg-cyan-700'
										: 'right-1 left-[calc(50%+2px)] bg-amber-500'
								}`}
							></div>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold transition-colors duration-200 disabled:opacity-40 ${$mode !== 'manual' ? 'text-white' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									mode.set('auto');
									waterSystem.sendCommand('auto');
								}}>Auto</button
							>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold transition-colors duration-200 disabled:opacity-40 ${$mode === 'manual' ? 'text-slate-950' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									mode.set('manual');
									waterSystem.sendCommand('manual');
								}}>Manual</button
							>
						</div>
					</div>

					<div
						class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
					>
						<p class="mb-3 text-xs font-semibold tracking-[0.15em] text-slate-500 uppercase">
							Pump
						</p>
						<div class="relative flex h-11 overflow-hidden rounded-2xl bg-slate-200 p-1">
							<div
								class={`absolute top-1 bottom-1 rounded-xl shadow-sm transition-all duration-200 ${
									$pumpPreference === 'borewell'
										? 'right-[calc(50%+2px)] left-1 bg-emerald-700'
										: 'right-1 left-[calc(50%+2px)] bg-lime-600'
								}`}
							></div>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold transition-colors duration-200 disabled:opacity-40 ${$pumpPreference === 'borewell' ? 'text-white' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									pumpPreference.set('borewell');
									waterSystem.sendCommand('force borewell');
								}}>Borewell</button
							>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold transition-colors duration-200 disabled:opacity-40 ${$pumpPreference === 'sump' ? 'text-slate-950' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									pumpPreference.set('sump');
									waterSystem.sendCommand('force sump');
								}}>Sump</button
							>
						</div>
					</div>

					<div class="col-span-2 sm:col-span-1">
						<button
							class={`min-h-28 w-full rounded-[1.5rem] border px-4 py-4 text-left shadow-sm transition active:scale-[0.99] disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-200 disabled:text-slate-500 ${
								($waterSystem.device?.override ?? false)
									? 'border-orange-600/20 bg-orange-500 text-white'
									: overrideSending
										? 'animate-pulse border-sky-500/20 bg-sky-500 text-white'
										: 'border-sky-700/10 bg-sky-700 text-white'
							}`}
							disabled={!isConnected || ($waterSystem.device?.override ?? false)}
							onclick={handleOverride}
						>
							<p class="text-sm font-semibold">
								{($waterSystem.device?.override ?? false)
									? 'Filling to High…'
									: commandLabels['override']}
							</p>
							<p class="mt-1 text-xs opacity-75">
								{($waterSystem.device?.override ?? false)
									? 'Resets automatically when overhead reaches High'
									: 'Force fill regardless of current level'}
							</p>
						</button>
					</div>
				</div>
			</section>
		</main>
	</div>
</div>
