<script lang="ts">
	import { onMount } from 'svelte';
	import { pumpPreference } from '../lib/stores/pump.js';
	import { mode } from '../lib/stores/mode.js';
	import { waterSystem } from '../lib/stores/system.js';
	import { commandLabels, runtimeStatusLabels } from '../lib/control.js';
	import type {
		ConnectionPhase,
		OverheadLevel,
		SumpLevel,
		MotorRuntimeStatus
	} from '../lib/types.js';
	import { connectionIconsMap } from '../lib/types.js';
	import { theme, themeIcons, type ThemePreference } from '$lib/stores/theme.js';
	import { icons, LoaderCircle, LoaderIcon } from 'lucide-svelte';

	let overrideSending = false;
	let eStopped = false;

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
		return theme.initialize();
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

	function pumpBodyTone(status: MotorRuntimeStatus): string {
		switch (status) {
			case 'running':
				return 'bg-emerald-500';
			case 'starting':
				return 'bg-amber-400';
			case 'dry_run_lock':
			case 'sump_critical':
				return 'bg-rose-500';
			default:
				return 'bg-slate-300';
		}
	}

	function pumpRingTone(status: MotorRuntimeStatus): string {
		switch (status) {
			case 'running':
				return 'border-emerald-300';
			case 'starting':
				return 'border-amber-300';
			case 'dry_run_lock':
			case 'sump_critical':
				return 'border-rose-300';
			default:
				return 'border-slate-300';
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
		eStopped = active;
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
			class={`overflow-hidden rounded-[2rem] border p-5 shadow-sm backdrop-blur-sm ${connectionShells[$waterSystem.connection.phase]}`}
		>
			<div class="flex flex-col">
				<div class="flex max-w-full items-center justify-between gap-3">
					<div class="flex items-center gap-2">
						<p class="text-[0.65rem] font-semibold tracking-[0.28em] text-cyan-900 uppercase">
							Water Level Automation
						</p>
						<span
							class={`rounded-full px-1 py-1 text-xs font-semibold tracking-wide uppercase ${connectionBadges[$waterSystem.connection.phase]}`}
						>
							<svelte:component
								this={connectionIconsMap[$waterSystem.connection.phase]}
								size={16}
								class={connectionIconsMap[$waterSystem.connection.phase] === LoaderCircle ? 'animate-spin': ''}
							/>
						</span>
					</div>
					<div class="flex items-center gap-2">
						<!-- 3-way pill: Sun / Monitor / Moon -->
						<div class="flex overflow-hidden rounded-full bg-black/10 p-0.5">
							{#each themeIcons as { val, icon: Icon }}
								<button
									onclick={() => theme.set(val as ThemePreference)}
									aria-label={val}
									class={`flex items-center justify-center rounded-full p-1.5 transition-colors duration-200
                    ${
											$theme === val
												? 'bg-white/90 text-slate-900 shadow-sm'
												: 'text-white/60 hover:text-white/90'
										}`}
								>
									<Icon size={16} />
								</button>
							{/each}
						</div>

						<!-- Existing connection badge, unchanged -->
					</div>
				</div>
				<div class="mt-4 flex max-w-full items-center justify-between gap-3">
					<h1 class="text-3xl font-semibold tracking-tight uppercase sm:text-4xl">Neptune</h1>
					{#if $waterSystem.device?.motors.borewell.status !== 'stopped' || $waterSystem.device?.motors.sump.status !== 'stopped'}
						{#if eStopped === false}
							<button
								class="rounded-2xl bg-rose-200 px-3 py-3 text-xs font-semibold tracking-wide text-rose-900 uppercase"
								onclick={() => eStop(true)}
							>
								{commandLabels['estop']}
							</button>
						{:else}
							<button
								class="rounded-2xl bg-emerald-200 px-3 py-3 text-xs font-semibold tracking-wide text-emerald-900 uppercase"
								onclick={() => eStop(false)}
							>
								{commandLabels['resume']}
							</button>
						{/if}
					{/if}
				</div>
			</div>
		</header>

		<main class="grid gap-4">
			<section class="rounded-[2rem] border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
					<div>
						<h2 class="text-xl font-semibold uppercase">Overview</h2>
					</div>
					{#if $waterSystem.device}
						<p class="text-xs text-slate-500">
							Last update {formatTimestamp($waterSystem.device.receivedAt)}
						</p>
					{/if}
				</div>

				{#if $waterSystem.device}
					<!-- ── TANKS ── -->
					<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
						Tanks
					</p>
					<div class="mt-2 grid gap-3 sm:grid-cols-2">
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
										Overhead
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950 uppercase">
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
										Sump
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950 uppercase">
										{sumpLabels[$waterSystem.device.sump]}
									</p>
								</div>
							</div>
						</div>
					</div>
					<div class="mt-6 h-1 rounded-full bg-slate-200"></div>
					<!-- ── PUMPS ── -->
					<p class="mt-4 text-base font-semibold tracking-[0.2em] text-slate-400 uppercase">
						Pumps
					</p>
					<div class="mt-2 grid gap-3 sm:grid-cols-2">
						<!-- BOREWELL PUMP CARD -->
						<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
							<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
								<!-- Pump graphic -->
								<div class="relative mx-auto flex h-28 w-28 items-center justify-center">
									<!-- Outer ring — acts as the pump casing -->
									<div
										class={`absolute inset-0 rounded-full border-[6px] bg-white shadow-inner transition-colors duration-500 ${pumpRingTone($waterSystem.device.motors.borewell.status)}`}
									>
										<!-- Impeller disc — spins when running, pulses via ring when starting -->
										<div
											class={`absolute inset-3 rounded-full transition-colors duration-500 ${pumpBodyTone($waterSystem.device.motors.borewell.status)} ${$waterSystem.device.motors.borewell.status === 'running' ? 'animate-spin' : ''}`}
											style="animation-duration: 1.4s"
										>
											<!-- Blade lines crossing through the disc center -->
											<!-- overflow-hidden + rounded-full clips them to the circle shape -->
											<div class="absolute inset-0 overflow-hidden rounded-full">
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
												></div>
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
													style="transform: translateY(-50%) rotate(60deg)"
												></div>
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
													style="transform: translateY(-50%) rotate(-60deg)"
												></div>
											</div>
										</div>

										<!-- Centre hub dot — stays still even when the impeller spins -->
										<div class="absolute inset-[38%] rounded-full bg-white shadow"></div>
									</div>

									<!-- Pulsing amber ring shown only while motor is 'starting' -->
									{#if $waterSystem.device.motors.borewell.status === 'starting'}
										<div
											class="absolute inset-0 animate-ping rounded-full border-4 border-amber-400 opacity-60"
										></div>
									{/if}
								</div>

								<!-- Text info -->
								<div>
									<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
										Borewell
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950 uppercase">
										{runtimeStatusLabels[$waterSystem.device.motors.borewell.status]}
									</p>
								</div>
							</div>
						</div>

						<!-- SUMP PUMP CARD — identical structure, different motor key -->
						<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
							<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
								<div class="relative mx-auto flex h-28 w-28 items-center justify-center">
									<div
										class={`absolute inset-0 rounded-full border-[6px] bg-white shadow-inner transition-colors duration-500 ${pumpRingTone($waterSystem.device.motors.sump.status)}`}
									>
										<div
											class={`absolute inset-3 rounded-full transition-colors duration-500 ${pumpBodyTone($waterSystem.device.motors.sump.status)} ${$waterSystem.device.motors.sump.status === 'running' ? 'animate-spin' : ''}`}
											style="animation-duration: 1.4s"
										>
											<div class="absolute inset-0 overflow-hidden rounded-full">
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
												></div>
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
													style="transform: translateY(-50%) rotate(60deg)"
												></div>
												<div
													class="absolute top-1/2 right-0 left-0 h-[3px] -translate-y-1/2 rounded-full bg-white/40"
													style="transform: translateY(-50%) rotate(-60deg)"
												></div>
											</div>
										</div>
										<div class="absolute inset-[38%] rounded-full bg-white shadow"></div>
									</div>
									{#if $waterSystem.device.motors.sump.status === 'starting'}
										<div
											class="absolute inset-0 animate-ping rounded-full border-4 border-amber-400 opacity-60"
										></div>
									{/if}
								</div>

								<div>
									<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
										Sump
									</p>
									<p class="mt-3 text-3xl font-semibold text-slate-950 uppercase">
										{runtimeStatusLabels[$waterSystem.device.motors.sump.status]}
									</p>
								</div>
							</div>
						</div>
					</div>
				{/if}
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
						<div class="relative flex h-11 overflow-hidden rounded-2xl bg-slate-200 p-1">
							<div
								class={`absolute top-1 bottom-1 rounded-xl shadow-sm transition-all duration-200 ${
									$mode !== 'manual'
										? 'right-[calc(50%+2px)] left-1 bg-cyan-700'
										: 'right-1 left-[calc(50%+2px)] bg-amber-500'
								}`}
							></div>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold uppercase transition-colors duration-200 disabled:opacity-40 ${$mode !== 'manual' ? 'text-white' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									mode.set('auto');
									waterSystem.sendCommand('auto');
								}}>Auto</button
							>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold uppercase transition-colors duration-200 disabled:opacity-40 ${$mode === 'manual' ? 'text-slate-950' : 'text-slate-500'}`}
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
						<p class="mb-3 text-base font-semibold tracking-[0.15em] text-slate-500 uppercase">
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
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold uppercase transition-colors duration-200 disabled:opacity-40 ${$pumpPreference === 'borewell' ? 'text-white' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									pumpPreference.set('borewell');
									waterSystem.sendCommand('force borewell');
								}}>Borewell</button
							>
							<button
								class={`relative z-10 flex-1 rounded-xl text-sm font-semibold uppercase transition-colors duration-200 disabled:opacity-40 ${$pumpPreference === 'sump' ? 'text-slate-950' : 'text-slate-500'}`}
								disabled={!isConnected}
								onclick={() => {
									pumpPreference.set('sump');
									waterSystem.sendCommand('force sump');
								}}>Sump</button
							>
						</div>
					</div>
					<div
						class="col-span-2 rounded-[1.5rem] border border-slate-200 bg-slate-50 p-4 shadow-sm sm:col-span-1"
					>
						<p class="mb-3 text-base font-semibold tracking-[0.15em] text-slate-500 uppercase">
							Override
						</p>
						<button
							aria-label="Override"
							class={`flex h-11 w-full items-center justify-center rounded-2xl border text-left font-semibold uppercase shadow-sm transition active:scale-[0.99] disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-200 disabled:text-slate-500 ${
								($waterSystem.device?.override ?? false)
									? 'border-orange-600/20 bg-orange-500 text-white'
									: overrideSending
										? 'animate-pulse border-sky-500/20 bg-sky-500 text-white'
										: 'border-sky-700/10 bg-sky-700 text-white'
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
	</div>
</div>
