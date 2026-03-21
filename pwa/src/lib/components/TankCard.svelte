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

	// Narrow the type so TS is happy when indexing the record
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

	const label = $derived(
		level == null
			? '—'
			: isOverhead
				? overheadLabels[level as OverheadLevel]
				: sumpLabels[level as SumpLevel]
	);

	// Overhead has 4 markers, sump has 3
	const markers = $derived(isOverhead ? ['High', 'Med', 'Low', 'Crit'] : ['High', 'Low', 'Crit']);
</script>

<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
	<div class="grid gap-4 sm:grid-cols-[7.5rem,1fr] sm:items-center">
		<!-- Tank graphic -->
		<div class="relative mx-auto {isOverhead ? 'h-56' : 'h-56'} w-28">
			<!-- Overhead neck cap / sump inlet hood -->
			{#if isOverhead}
				<div class="absolute inset-x-5 top-0 h-4 rounded-full bg-slate-300/90"></div>
			{:else}
				<div
					class="absolute inset-x-4 top-0 h-6 rounded-t-[1.4rem] border-4 border-b-0 border-slate-300 bg-slate-100"
				></div>
			{/if}

			<!-- Tank body -->
			<div
				class="absolute inset-x-0 {isOverhead
					? 'top-3'
					: 'top-5'} bottom-0 overflow-hidden rounded-[2rem] border-[6px] border-slate-300 bg-white shadow-inner"
			>
				<!-- Level markers -->
				<div
					class="pointer-events-none absolute inset-x-3 top-5 bottom-5 flex flex-col justify-between"
				>
					{#each markers.slice(0, -1) as _}
						<div class="border-t border-dashed border-slate-300/80"></div>
					{/each}
				</div>

				<!-- Water fill -->
				<div
					class="absolute inset-x-0 bottom-0 overflow-hidden rounded-b-[1.6rem] bg-gradient-to-t transition-[height] duration-700 {tone}"
					style="height: {fillHeight};"
				>
					<!-- Hatching texture -->
					<div
						class="absolute inset-0 opacity-30"
						style="background-image: linear-gradient(135deg, rgba(255,255,255,0.25) 25%, transparent 25%, transparent 50%, rgba(255,255,255,0.25) 50%, rgba(255,255,255,0.25) 75%, transparent 75%); background-size: 20px 20px;"
					></div>
					<!-- Specular highlight -->
					<div class="absolute inset-x-4 top-2 h-2 rounded-full bg-white/35 blur-[1px]"></div>
					<!-- Wave shimmer at the surface -->
					<div class="wave-shimmer absolute inset-x-0 top-0 h-3 opacity-40"></div>
				</div>
			</div>

			<!-- Level labels -->
			<div
				class="absolute top-{isOverhead ? '8' : '10'} -right-14 flex {isOverhead
					? 'h-44'
					: 'h-40'} pointer-events-none flex-col justify-between text-[0.65rem] font-semibold tracking-[0.2em] text-slate-400 uppercase"
			>
				{#each markers as m}
					<span>{m}</span>
				{/each}
			</div>
		</div>

		<!-- Label and value -->
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
