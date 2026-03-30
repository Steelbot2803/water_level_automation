<svelte:options runes={true} />

<script lang="ts">
	import { onDestroy } from 'svelte';
	import { fade, fly } from 'svelte/transition';
	import { CircleCheckBig, CircleAlert, Info, TriangleAlert, X } from 'lucide-svelte';
	import { alerts } from '$lib/stores/alerts.js';

	const severityStyles = {
		success: 'border-emerald-200 bg-white/96 text-slate-950',
		error: 'border-rose-200 bg-white/96 text-slate-950',
		info: 'border-sky-200 bg-white/96 text-slate-950',
		warning: 'border-amber-200 bg-white/96 text-slate-950'
	} as const;

	// Progress bar color per severity — used as a CSS custom property so the
	// bar animates purely in CSS with no JS involvement.
	const progressColors = {
		success: '#10b981',
		error: '#f43f5e',
		info: '#0ea5e9',
		warning: '#f59e0b'
	} as const;

	const iconStyles = {
		success: 'border-emerald-200 bg-emerald-50 text-emerald-700',
		error: 'border-rose-200 bg-rose-50 text-rose-700',
		info: 'border-sky-200 bg-sky-50 text-sky-700',
		warning: 'border-amber-200 bg-amber-50 text-amber-700'
	} as const;

	const severityIcons = {
		success: CircleCheckBig,
		error: CircleAlert,
		info: Info,
		warning: TriangleAlert
	} as const;

	// No rAF loop needed. Each bar is a <div> whose width animates from 100%
	// to 0% over `duration` ms using a CSS linear transition. Pausing is done
	// by toggling animation-play-state via a data attribute — zero JS per frame.
	function progressStyle(alert: (typeof $alerts)[number]) {
		return [
			`--dur: ${alert.duration}ms`,
			`--remaining: ${alert.remaining}ms`,
			`background: ${progressColors[alert.severity]}`
		].join(';');
	}
</script>

<div
	class="pointer-events-none fixed inset-x-4 top-[calc(1rem+env(safe-area-inset-top))] z-50 flex flex-col gap-3 md:right-4 md:left-auto md:w-full md:max-w-sm"
	aria-live="polite"
	aria-atomic="true"
>
	{#each $alerts as alert (alert.id)}
		{@const Icon = severityIcons[alert.severity]}
		<div
			role="status"
			class={`pointer-events-auto relative overflow-hidden rounded-[1.6rem] border px-4 pt-4 pb-3 shadow-[0_18px_40px_-18px_rgba(15,23,42,0.35)] ${severityStyles[alert.severity]}`}
			onmouseenter={() => alerts.pause(alert.id)}
			onmouseleave={() => alerts.resume(alert.id)}
			data-paused={alert.timer ? undefined : ''}
			in:fly={{ y: -16, duration: 180 }}
			out:fade={{ duration: 180 }}
		>
			<div class="flex items-start justify-between gap-3">
				<div class="flex min-w-0 gap-3">
					<div
						class={`flex h-10 w-10 shrink-0 items-center justify-center rounded-full border shadow-sm ${iconStyles[alert.severity]}`}
					>
						<Icon size={18} />
					</div>
					<div class="min-w-0 pt-0.5">
						<p class="text-sm font-semibold tracking-[0.08em] uppercase">{alert.title}</p>
						<p class="mt-1 text-sm leading-5 text-slate-600">{alert.message}</p>
					</div>
				</div>
				<button
					class="flex h-9 w-9 shrink-0 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition hover:bg-slate-100 hover:text-slate-700 active:scale-[0.98]"
					onclick={() => alerts.dismiss(alert.id)}
					aria-label="Dismiss notification"
				>
					<X size={16} />
				</button>
			</div>

			<!--
				Progress bar driven entirely by CSS.
				`animation-duration` is set to the full toast duration.
				`animation-delay` is negative: -（duration - remaining) rewinds
				the animation to exactly where it left off after a pause/resume.
				`animation-play-state` is toggled via the parent's data-paused attribute.
			-->
			<div class="mt-3 h-1.5 overflow-hidden rounded-full bg-slate-200/80">
				<div class="progress-bar h-full rounded-full" style={progressStyle(alert)}></div>
			</div>
		</div>
	{/each}
</div>

<style>
	@keyframes shrink {
		from {
			width: 100%;
		}
		to {
			width: 0%;
		}
	}

	.progress-bar {
		animation: shrink var(--dur) linear forwards;
		/* Negative delay rewinds the animation to match remaining time after resume. */
		animation-delay: calc(-1 * (var(--dur) - var(--remaining)));
	}

	/* Pause the bar when the parent toast has data-paused set. */
	[data-paused] .progress-bar {
		animation-play-state: paused;
	}
</style>
