<svelte:options runes={true} />

<script lang="ts">
	import type { OverheadLevel, SumpLevel } from '$lib/types.js';

	type TankVariant = 'overhead' | 'sump';

	let {
		variant,
		level
	}: {
		variant: TankVariant;
		level: OverheadLevel | SumpLevel | undefined;
	} = $props();

	const overheadFillHeights: Record<OverheadLevel, string> = {
		critical: '8%',
		low: '30%',
		medium: '58%',
		high: '88%'
	};

	const sumpFillHeights: Record<SumpLevel, string> = {
		critical: '8%',
		low: '48%',
		high: '88%'
	};

	const overheadTones: Record<OverheadLevel, string> = {
		high: 'from-emerald-400 via-emerald-500 to-emerald-700',
		medium: 'from-cyan-400 via-sky-500 to-sky-700',
		low: 'from-amber-300 via-amber-400 to-orange-500',
		critical: 'from-rose-400 via-rose-500 to-rose-700'
	};

	const sumpTones: Record<SumpLevel, string> = {
		high: 'from-emerald-400 via-teal-500 to-teal-700',
		low: 'from-amber-300 via-yellow-400 to-orange-500',
		critical: 'from-rose-400 via-rose-500 to-rose-700'
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

	const isOverhead = $derived(variant === 'overhead');

	const fillHeight = $derived(
		level == null
			? '0%'
			: isOverhead
				? overheadFillHeights[level as OverheadLevel]
				: sumpFillHeights[level as SumpLevel]
	);

	const tone = $derived(
		level == null
			? ''
			: isOverhead
				? overheadTones[level as OverheadLevel]
				: sumpTones[level as SumpLevel]
	);

	const shellTone = $derived(
		level === 'critical'
			? 'border-rose-300'
			: level === 'low'
				? 'border-amber-300'
				: level === 'medium'
					? 'border-sky-300'
					: level === 'high'
						? 'border-emerald-300'
						: 'border-slate-300'
	);

	const label = $derived(
		level == null
			? '--'
			: isOverhead
				? overheadLabels[level as OverheadLevel]
				: sumpLabels[level as SumpLevel]
	);

	const markers = $derived(isOverhead ? ['High', 'Med', 'Low', 'Crit'] : ['High', 'Low', 'Crit']);

	const tankGlow = $derived(
		level === 'critical'
			? 'shadow-[0_18px_28px_-24px_rgba(244,63,94,0.46)]'
			: level === 'low'
				? 'shadow-[0_18px_28px_-24px_rgba(245,158,11,0.42)]'
				: level === 'medium'
					? 'shadow-[0_18px_28px_-24px_rgba(14,165,233,0.42)]'
					: level === 'high'
						? 'shadow-[0_18px_28px_-24px_rgba(16,185,129,0.42)]'
						: 'shadow-[0_18px_28px_-24px_rgba(148,163,184,0.34)]'
	);
</script>

<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
	<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
		<div class="relative mx-auto h-56 w-28">
			{#if isOverhead}
				<div
					class="absolute inset-x-5 top-0 h-4 rounded-full bg-slate-300/90 shadow-[inset_0_1px_0_rgba(255,255,255,0.62)]"
				></div>
				<div class="absolute inset-x-7 top-[0.18rem] h-2 rounded-full bg-white/35 blur-[1px]"></div>
			{:else}
				<div
					class="absolute inset-x-4 top-0 h-6 rounded-t-[1.4rem] border-4 border-b-0 border-slate-300 bg-slate-100 shadow-[inset_0_1px_0_rgba(255,255,255,0.65)]"
				></div>
			{/if}

			<div
				class="absolute inset-x-0 {isOverhead ? 'top-3' : 'top-5'} bottom-0 overflow-hidden rounded-[2rem] border-[6px] bg-[linear-gradient(180deg,rgba(255,255,255,0.98),rgba(241,245,249,0.96))] shadow-[inset_0_1px_0_rgba(255,255,255,0.72),inset_0_-10px_18px_rgba(148,163,184,0.14)] {shellTone} {tankGlow}"
			>
				<div class="absolute inset-[0.32rem] rounded-[1.55rem] border border-white/70"></div>
				<div class="absolute inset-y-4 left-3 w-2 rounded-full bg-white/30 blur-[1px]"></div>
				<div class="absolute inset-y-5 right-2.5 w-px bg-slate-300/45"></div>

				<div
					class="pointer-events-none absolute inset-x-4 top-5 bottom-5 flex flex-col justify-between"
				>
					{#each markers.slice(0, -1) as _}
						<div
							class="h-px bg-[linear-gradient(90deg,transparent,rgba(148,163,184,0.8),transparent)]"
						></div>
					{/each}
				</div>

				<div
					class="absolute inset-x-[0.24rem] bottom-[0.24rem] overflow-hidden rounded-b-[1.42rem] bg-gradient-to-t transition-[height] duration-700 {tone}"
					style="height: {fillHeight};"
				>
					<div class="absolute inset-x-0 top-0 h-4 bg-white/12 blur-[2px]"></div>
					<div
						class="absolute inset-0 opacity-24"
						style="background-image: linear-gradient(135deg, rgba(255,255,255,0.24) 25%, transparent 25%, transparent 50%, rgba(255,255,255,0.24) 50%, rgba(255,255,255,0.24) 75%, transparent 75%); background-size: 20px 20px;"
					></div>
					<div class="wave-shimmer absolute inset-x-0 top-0 h-2 opacity-40"></div>
				</div>
			</div>

			<div
				class="absolute top-{isOverhead ? '8' : '10'} -right-14 flex {isOverhead ? 'h-44' : 'h-40'} pointer-events-none flex-col justify-between text-[0.65rem] font-semibold tracking-[0.2em] text-slate-400 uppercase"
			>
				{#each markers as m}
					<span class="rounded-full bg-white/80 px-2 py-0.5 text-center shadow-sm">{m}</span>
				{/each}
			</div>
		</div>

		<div>
			<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
				{isOverhead ? 'Overhead' : 'Sump'}
			</p>
			<p
				class="mt-3 text-3xl font-semibold uppercase"
				class:text-rose-600={level === 'critical'}
				class:text-amber-600={level === 'low'}
				class:text-cyan-600={level === 'medium'}
				class:text-emerald-600={level === 'high'}
			>
				{label}
			</p>
		</div>
	</div>
</div>

<style>
	@keyframes wave {
		0% {
			transform: translateX(0);
		}
		100% {
			transform: translateX(-50%);
		}
	}

	.wave-shimmer {
		background: repeating-linear-gradient(
			90deg,
			transparent 0px,
			rgba(255, 255, 255, 0.5) 10px,
			transparent 20px
		);
		background-size: 40px 100%;
		animation: wave 2s linear infinite;
		width: 200%;
	}
</style>
