<script lang="ts">
	import { CircleCheckBig, Lock, TriangleAlert } from 'lucide-svelte';
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
		resolvedStatus === 'stopped' ? 'bg-white/65' : 'bg-white/75'
	);
</script>

<svelte:options runes={true} />

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
					<!-- Blade lines, clipped to circle -->
					<div class="absolute inset-0 overflow-hidden rounded-full">
						<div
							class="absolute top-1/2 right-[0.45rem] left-[0.45rem] h-[4px] -translate-y-1/2 rounded-full {bladeTone}"
						></div>
						<div
							class="absolute top-1/2 right-[0.45rem] left-[0.45rem] h-[4px] -translate-y-1/2 rounded-full {bladeTone}"
							style="transform: translateY(-50%) rotate(60deg)"
						></div>
						<div
							class="absolute top-1/2 right-[0.45rem] left-[0.45rem] h-[4px] -translate-y-1/2 rounded-full {bladeTone}"
							style="transform: translateY(-50%) rotate(-60deg)"
						></div>
					</div>
				</div>

				<!-- Centre hub -->
				<div
					class="absolute inset-[35%] rounded-full border border-white/70 bg-white shadow-[0_2px_6px_rgba(15,23,42,0.12)]"
				></div>

				<!-- Lock/warning overlay for error states -->
				{#if isLocked}
					<div
						class="absolute inset-[0.62rem] rounded-full bg-rose-950/24 backdrop-blur-[1px]"
					>
						<div class="absolute top-1/2 left-1/2 -translate-x-[49%] -translate-y-[55%]">
							<StatusIcon
								size={26}
								class="text-white drop-shadow-[0_1px_1px_rgba(0,0,0,0.24)]"
							/>
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
			<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">{label}</p>
			<div class="mt-3 flex items-center justify-between gap-3">
				<p
					class="text-3xl font-semibold uppercase"
					class:text-emerald-600={resolvedStatus === 'running'}
					class:text-amber-600={resolvedStatus === 'starting'}
					class:text-rose-600={isLocked}
					class:text-slate-950={resolvedStatus === 'stopped'}
				>
					{runtimeStatusLabels[resolvedStatus]}
				</p>
				{#if resolvedStatus === 'dry_run_lock' && onUnlock}
					<button
						type="button"
						class="text-l shrink-0 rounded-full border border-rose-200 bg-rose-50 px-3 py-1 font-semibold tracking-[0.14em] text-rose-700 uppercase shadow-sm transition active:scale-[0.98] hover:bg-rose-100 disabled:pointer-events-none disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-100 disabled:text-slate-400 disabled:shadow-none"
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
