<script lang="ts">
	import { CircleCheckBig, Lock, TriangleAlert } from 'lucide-svelte';
	import type { MotorRuntimeStatus } from '$lib/types.js';
	import { runtimeStatusLabels } from '$lib/control.js';

	let {
		label,
		status
	}: {
		label: string;
		status: MotorRuntimeStatus | undefined;
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
					class="absolute inset-3 rounded-full transition-colors duration-500 {bodyTone}"
					class:animate-spin={isSpinning}
					style="animation-duration: 1.4s"
				>
					<!-- Blade lines, clipped to circle -->
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

				<!-- Centre hub -->
				<div class="absolute inset-[38%] rounded-full bg-white shadow"></div>

				<!-- Lock/warning overlay for error states -->
				{#if isLocked}
					<div
						class="absolute inset-0 flex items-center justify-center rounded-full bg-rose-900/20"
					>
						<StatusIcon size={20} class="text-rose-100" />
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
			<p
				class="mt-3 text-3xl font-semibold uppercase"
				class:text-emerald-600={resolvedStatus === 'running'}
				class:text-amber-600={resolvedStatus === 'starting'}
				class:text-rose-600={isLocked}
				class:text-slate-950={resolvedStatus === 'stopped'}
			>
				{runtimeStatusLabels[resolvedStatus]}
			</p>
			{#if resolvedStatus === 'dry_run_lock'}
				<p class="mt-1 text-xs font-medium text-rose-500">Send "unlock" to clear</p>
			{:else if resolvedStatus === 'sump_critical'}
				<p class="mt-1 text-xs font-medium text-rose-500">Sump level too low to run</p>
			{/if}
		</div>
	</div>
</div>
