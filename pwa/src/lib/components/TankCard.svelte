<svelte:options runes={true} />

<script lang="ts">
	import type { OverheadLevel, SumpLevel } from '$lib/types.js';
	import { LoaderCircle } from 'lucide-svelte';

	type TankVariant = 'overhead' | 'sump';

	let {
		variant,
		level
	}: {
		variant: TankVariant;
		level: OverheadLevel | SumpLevel | undefined;
	} = $props();

	// All per-level style values in one lookup — one $derived instead of seven,
	// recomputed only when `level` actually changes.
	type LevelStyles = {
		fillPct: number;
		tone: string;
		shellTone: string;
		textTone: string;
		waveVars: string;
		tankGlow: string;
	};

	const overheadStyles: Record<OverheadLevel, LevelStyles> = {
		critical: {
			fillPct: 8,
			tone: 'linear-gradient(to top, #9f1239, #fb7185)',
			shellTone: 'border-rose-300',
			textTone: 'text-rose-600',
			waveVars:
				'--wave-blob:rgba(251,113,133,0.22);--wave-streak:rgba(253,164,175,0.13);--wave-foam:rgba(255,228,230,0.55)',
			tankGlow: 'shadow-[0_18px_28px_-24px_rgba(244,63,94,0.46)]'
		},
		low: {
			fillPct: 30,
			tone: 'linear-gradient(to top, #b45309, #fcd34d)',
			shellTone: 'border-amber-300',
			textTone: 'text-amber-600',
			waveVars:
				'--wave-blob:rgba(252,211,77,0.22);--wave-streak:rgba(253,230,138,0.13);--wave-foam:rgba(255,251,235,0.55)',
			tankGlow: 'shadow-[0_18px_28px_-24px_rgba(245,158,11,0.42)]'
		},
		medium: {
			fillPct: 58,
			tone: 'linear-gradient(to top, #0369a1, #38bdf8)',
			shellTone: 'border-sky-300',
			textTone: 'text-cyan-600',
			waveVars:
				'--wave-blob:rgba(56,189,248,0.22);--wave-streak:rgba(125,211,252,0.13);--wave-foam:rgba(224,242,254,0.55)',
			tankGlow: 'shadow-[0_18px_28px_-24px_rgba(14,165,233,0.42)]'
		},
		high: {
			fillPct: 88,
			tone: 'linear-gradient(to top, #047857, #6ee7b7)',
			shellTone: 'border-emerald-300',
			textTone: 'text-emerald-600',
			waveVars:
				'--wave-blob:rgba(110,231,183,0.22);--wave-streak:rgba(167,243,208,0.13);--wave-foam:rgba(236,253,245,0.55)',
			tankGlow: 'shadow-[0_18px_28px_-24px_rgba(16,185,129,0.42)]'
		}
	};

	const sumpStyles: Record<SumpLevel, LevelStyles> = {
		critical: overheadStyles.critical,
		low: {
			fillPct: 48,
			tone: 'linear-gradient(to top, #b45309, #fcd34d)',
			shellTone: 'border-amber-300',
			textTone: 'text-amber-600',
			waveVars: overheadStyles.low.waveVars,
			tankGlow: overheadStyles.low.tankGlow
		},
		high: overheadStyles.high
	};

	const nullStyles: LevelStyles = {
		fillPct: 0,
		tone: '',
		shellTone: 'border-slate-300',
		textTone: 'text-slate-950',
		waveVars:
			'--wave-blob:rgba(148,163,184,0.15);--wave-streak:rgba(203,213,225,0.10);--wave-foam:rgba(241,245,249,0.40)',
		tankGlow: 'shadow-[0_18px_28px_-24px_rgba(148,163,184,0.34)]'
	};

	const isOverhead = $derived(variant === 'overhead');

	// Single lookup — runs only when `level` or `variant` changes.
	const styles = $derived(
		level == null
			? nullStyles
			: isOverhead
				? overheadStyles[level as OverheadLevel]
				: sumpStyles[level as SumpLevel]
	);
</script>

<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
	<div class="flex flex-col items-center gap-3">
		<div class="relative h-55 w-29 items-center justify-center">
			{#if isOverhead}
				<div
					class="absolute inset-x-5 top-0 h-4 rounded-full bg-slate-300/90 shadow-[inset_0_1px_0_rgba(255,255,255,0.62)]"
				></div>
				<div class="absolute inset-x-7 top-[0.18rem] h-1 rounded-full bg-white/35 blur-[1px]"></div>
			{:else}
				<div
					class="absolute inset-x-4 top-0 h-6 rounded-t-[1.4rem] border-4 border-b-0 border-slate-300 bg-slate-100 shadow-[inset_0_1px_0_rgba(255,255,255,0.65)]"
				></div>
			{/if}

			<div
				class="absolute inset-x-0 {isOverhead
					? 'top-3'
					: 'top-5'} bottom-0 overflow-hidden rounded-[2rem] border-[6px] bg-[linear-gradient(180deg,rgba(255,255,255,0.98),rgba(241,245,249,0.96))] {styles.shellTone} {styles.tankGlow}"
			>
				<div class="absolute inset-[0.32rem] rounded-[1.55rem] border border-white/70"></div>
				<div class="absolute inset-y-4 left-3 w-2 rounded-full bg-white/30 blur-[1px]"></div>

				<div
					class="absolute inset-x-[0.24rem] bottom-[0.24rem] overflow-hidden rounded-b-[1.42rem] transition-[height,background] duration-700"
					style="height:{styles.fillPct}%;background:{styles.tone};{styles.waveVars}"
				>
					<div class="wave-swell absolute inset-0"></div>
					<div class="wave-ripple absolute inset-0"></div>
					<div class="wave-foam absolute inset-x-0 top-0 h-5"></div>
				</div>
			</div>

			{#if level == null}
				<div class="absolute inset-0 flex items-center justify-center">
					<LoaderCircle size={70} class="animate-spin text-slate-300" />
				</div>
			{/if}
		</div>

		<div class="pointer-events-none text-center">
			<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">
				{isOverhead ? 'Overhead' : 'Sump'}
			</p>
			<p
				class="mt-1 text-2xl font-semibold uppercase transition-colors duration-700 {styles.textTone}"
			>
				{level ?? '--'}
			</p>
		</div>
	</div>
</div>

<style>
	@keyframes wave-drift {
		from {
			background-position: 0% 50%;
		}
		to {
			background-position: 200% 50%;
		}
	}

	@keyframes wave-pulse {
		0%,
		100% {
			opacity: 0.45;
		}
		50% {
			opacity: 0.75;
		}
	}

	.wave-swell {
		background:
			radial-gradient(ellipse 60% 40% at 25% 55%, var(--wave-blob) 0%, transparent 70%),
			radial-gradient(ellipse 50% 35% at 75% 45%, var(--wave-blob) 0%, transparent 65%),
			radial-gradient(ellipse 40% 30% at 50% 60%, var(--wave-blob) 0%, transparent 60%);
		background-size: 200% 200%;
		animation: wave-drift 14s linear infinite;
		filter: blur(4px);
	}

	.wave-ripple {
		background: repeating-linear-gradient(
			165deg,
			transparent 0px,
			var(--wave-streak) 4px,
			var(--wave-streak) 8px,
			transparent 16px
		);
		background-size: 200% 100%;
		animation: wave-drift 7s linear infinite;
		filter: blur(0.6px);
		opacity: 0.6;
	}

	.wave-foam {
		background: linear-gradient(to bottom, var(--wave-foam) 0%, transparent 100%);
		filter: blur(3px);
		animation: wave-pulse 3s ease-in-out infinite;
	}
</style>
