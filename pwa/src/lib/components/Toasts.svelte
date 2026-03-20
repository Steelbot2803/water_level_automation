<script lang="ts">
	import { onDestroy } from 'svelte';
	import { fade, fly } from 'svelte/transition';
	import { CheckCircle2, CircleAlert, Info, TriangleAlert, X } from 'lucide-svelte';

	import { alerts } from '$lib/stores/alerts.js';

	const severityStyles = {
		success: 'border-emerald-200 bg-white/96 text-slate-950',
		error: 'border-rose-200 bg-white/96 text-slate-950',
		info: 'border-sky-200 bg-white/96 text-slate-950',
		warning: 'border-amber-200 bg-white/96 text-slate-950'
	} as const;

	const progressStyles = {
		success: 'bg-emerald-500',
		error: 'bg-rose-500',
		info: 'bg-sky-500',
		warning: 'bg-amber-500'
	} as const;

	const iconStyles = {
		success: 'border-emerald-200 bg-emerald-50 text-emerald-700',
		error: 'border-rose-200 bg-rose-50 text-rose-700',
		info: 'border-sky-200 bg-sky-50 text-sky-700',
		warning: 'border-amber-200 bg-amber-50 text-amber-700'
	} as const;

	const severityIcons = {
		success: CheckCircle2,
		error: CircleAlert,
		info: Info,
		warning: TriangleAlert
	} as const;

	let now = $state(Date.now());
	let ticker: ReturnType<typeof setInterval> | null = null;

	function ensureTicker(active: boolean) {
		if (active) {
			if (!ticker) {
				ticker = setInterval(() => {
					now = Date.now();
				}, 100);
			}
			return;
		}

		if (ticker) {
			clearInterval(ticker);
			ticker = null;
		}
	}

	function progressPercent(alert: (typeof $alerts)[number]) {
		if (!alert.duration) return 0;

		const remaining =
			alert.timer && alert.startedAt
				? Math.max(0, alert.remaining - (now - alert.startedAt))
				: alert.remaining;

		return Math.max(0, Math.min(100, (remaining / alert.duration) * 100));
	}

	$effect(() => {
		ensureTicker($alerts.length > 0);
	});

	onDestroy(() => {
		ensureTicker(false);
	});
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
			class={`pointer-events-auto relative overflow-hidden rounded-[1.6rem] border px-4 pt-4 pb-3 shadow-[0_18px_40px_-18px_rgba(15,23,42,0.35)] backdrop-blur-sm ${severityStyles[alert.severity]}`}
			onmouseenter={() => alerts.pause(alert.id)}
			onmouseleave={() => alerts.resume(alert.id)}
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
					class="flex h-9 w-9 shrink-0 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition active:scale-[0.98] hover:bg-slate-100 hover:text-slate-700"
					onclick={() => alerts.dismiss(alert.id)}
					aria-label="Dismiss notification"
				>
					<X size={16} />
				</button>
			</div>

			<div class="mt-3 h-1.5 overflow-hidden rounded-full bg-slate-200/80">
				<div
					class={`h-full rounded-full transition-[width] duration-100 linear ${progressStyles[alert.severity]}`}
					style={`width: ${progressPercent(alert)}%;`}
				></div>
			</div>
		</div>
	{/each}
</div>
