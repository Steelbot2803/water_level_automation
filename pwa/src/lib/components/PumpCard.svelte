<svelte:options runes={true} />

<script lang="ts">
	import { Aperture, CircleCheckBig, Lock, TriangleAlert } from 'lucide-svelte';
	import type { MotorRuntimeStatus } from '$lib/types.js';
	import { runtimeStatusLabels } from '$lib/control.js';

	let {
		label,
		status,
		onUnlock = undefined,
		unlockDisabled = false
	}: {
		label: string;
		status: MotorRuntimeStatus | undefined;
		onUnlock?: (() => void) | undefined;
		unlockDisabled?: boolean;
	} = $props();

	const resolvedStatus = $derived(status ?? 'stopped');

	const StatusIcon = $derived(
		resolvedStatus === 'dry_run_lock'
			? Lock
			: resolvedStatus === 'sump_critical'
				? TriangleAlert
				: CircleCheckBig
	);

	const bodyTone = $derived(
		resolvedStatus === 'running'
			? 'bg-emerald-500'
			: resolvedStatus === 'starting'
				? 'bg-amber-400'
				: resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
					? 'bg-rose-500'
					: 'bg-slate-300'
	);

	const ringTone = $derived(
		resolvedStatus === 'running'
			? 'border-emerald-300'
			: resolvedStatus === 'starting'
				? 'border-amber-300'
				: resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
					? 'border-rose-300'
					: 'border-slate-300'
	);

	const isSpinning = $derived(resolvedStatus === 'running');
	const isPulsing = $derived(resolvedStatus === 'starting');
	const isLocked = $derived(
		resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
	);
	const bladeTone = $derived(
		resolvedStatus === 'stopped'
			? 'text-slate-900/72'
			: resolvedStatus === 'starting'
				? 'text-amber-950/60'
				: resolvedStatus === 'running'
					? 'text-emerald-950/55'
					: 'text-rose-950/60'
	);
	const hubTone = $derived(
		resolvedStatus === 'running'
			? 'bg-emerald-100'
			: resolvedStatus === 'starting'
				? 'bg-amber-100'
				: resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
					? 'bg-rose-100'
					: 'bg-white'
	);
	const impellerShellTone = $derived(
		resolvedStatus === 'running'
			? 'from-emerald-400/20 via-transparent to-slate-950/10'
			: resolvedStatus === 'starting'
				? 'from-amber-300/22 via-transparent to-slate-950/10'
				: resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
					? 'from-rose-400/18 via-transparent to-slate-950/10'
					: 'from-white/22 via-transparent to-slate-950/10'
	);
</script>

<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
	<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
		<!-- Pump graphic -->
		<div class="relative mx-auto flex h-28 w-28 items-center justify-center">
			<!-- Outer ring (casing) -->
			<div
				class="absolute inset-0 rounded-full border-[6px] bg-white shadow-inner transition-colors duration-500 {ringTone}"
			>
				<!-- Impeller disc -->
				<div
					class="absolute inset-[0.62rem] rounded-full border border-white/35 shadow-[inset_0_1px_0_rgba(255,255,255,0.35)] transition-colors duration-500 {bodyTone}"
					class:animate-spin={isSpinning}
					style="animation-duration: 1.4s"
				>
					<div
						class="absolute inset-[0.3rem] rounded-full bg-[linear-gradient(145deg,var(--tw-gradient-stops))] {impellerShellTone}"
					></div>
					<div class="absolute inset-[0.45rem] rounded-full border border-white/12"></div>
					<div class="absolute inset-[1.05rem] rounded-full border border-slate-950/10"></div>
					<Aperture
						size={80}
						strokeWidth={2}
						class="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 rotate-[14deg] {bladeTone} drop-shadow-[0_1px_1px_rgba(255,255,255,0.12)]"
					/>
					<div
						class="absolute top-[1.05rem] left-[1.35rem] h-[1.1rem] w-[2.15rem] rotate-[28deg] rounded-full bg-white/10 blur-[1px]"
					></div>
					<div
						class="absolute top-1/2 left-1/2 h-[2.5rem] w-[2.5rem] -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/60 {hubTone} shadow-[0_2px_7px_rgba(15,23,42,0.18)]"
					></div>
					<div
						class="absolute top-1/2 left-1/2 h-[1.5rem] w-[1.5rem] -translate-x-1/2 -translate-y-1/2 rounded-full border border-slate-950/10 bg-slate-950"
					></div>
				</div>

				<!-- Centre hub -->
				<div
					class="absolute inset-[34%] rounded-full border border-white/70 bg-white/55 shadow-[0_2px_6px_rgba(15,23,42,0.12)]"
				></div>

				<!-- Lock/warning overlay for error states -->
				{#if isLocked}
					<div class="absolute inset-[0.62rem] rounded-full bg-rose-950/24 backdrop-blur-[1px]">
						<div class="absolute top-1/2 left-1/2 -translate-x-[49%] -translate-y-[55%]">
							<StatusIcon size={26} class="text-white drop-shadow-[0_1px_1px_rgba(0,0,0,0.24)]" />
						</div>
					</div>
				{/if}
			</div>

			<!-- Pulsing ring when starting -->
			{#if isPulsing}
				<div
					class="absolute inset-0 animate-ping rounded-full border-4 border-amber-400 opacity-60"
				></div>
			{/if}
		</div>

		<!-- Text info -->
		<div>
			<div class="pointer-events-none">
				<p class="text-s font-semibold tracking-[0.2em] text-slate-500 uppercase">{label}</p>
			</div>
			<div class="mt-3 flex items-center justify-between gap-3">
				<div class="pointer-events-none">
					<p
						class="text-3xl font-semibold uppercase"
						class:text-emerald-600={resolvedStatus === 'running'}
						class:text-amber-600={resolvedStatus === 'starting'}
						class:text-rose-600={isLocked}
						class:text-slate-950={resolvedStatus === 'stopped'}
					>
						{runtimeStatusLabels[resolvedStatus]}
					</p>
				</div>
				{#if resolvedStatus === 'dry_run_lock' && onUnlock}
					<button
						type="button"
						class="text-l shrink-0 rounded-full border border-rose-200 bg-rose-50 px-3 py-1 font-semibold tracking-[0.14em] text-rose-700 uppercase shadow-sm transition hover:bg-rose-100 active:scale-[0.98] disabled:pointer-events-none disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-100 disabled:text-slate-400 disabled:shadow-none"
						disabled={unlockDisabled}
						onclick={onUnlock}
					>
						Unlock
					</button>
				{/if}
			</div>
		</div>
	</div>
</div>
