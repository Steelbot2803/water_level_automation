<script lang="ts">
	import { fade, fly } from 'svelte/transition';
	import { X } from 'lucide-svelte';

	import { alerts } from '$lib/stores/alerts.js';

	const severityStyles = {
		success: 'border-emerald-200 bg-emerald-50 text-emerald-950',
		error: 'border-rose-200 bg-rose-50 text-rose-950',
		info: 'border-sky-200 bg-sky-50 text-sky-950',
		warning: 'border-amber-200 bg-amber-50 text-amber-950'
	} as const;

	const progressStyles = {
		success: 'bg-emerald-500',
		error: 'bg-rose-500',
		info: 'bg-sky-500',
		warning: 'bg-amber-500'
	} as const;
</script>

<div
	class="pointer-events-none fixed inset-x-4 top-[calc(1rem+env(safe-area-inset-top))] z-50 flex flex-col gap-3 md:right-4 md:left-auto md:w-full md:max-w-sm"
	aria-live="polite"
	aria-atomic="true"
>
	{#each $alerts as alert (alert.id)}
		<div
			role="status"
			class={`pointer-events-auto relative overflow-hidden rounded-[1.25rem] border p-4 shadow-lg backdrop-blur-sm ${severityStyles[alert.severity]}`}
			onmouseenter={() => alerts.pause(alert.id)}
			onmouseleave={() => alerts.resume(alert.id)}
			in:fly={{ y: -16, duration: 180 }}
			out:fade={{ duration: 180 }}
		>
			<div class="flex items-start justify-between gap-3">
				<div class="min-w-0">
					<p class="text-sm font-semibold">{alert.title}</p>
					<p class="mt-1 text-sm leading-6 opacity-90">{alert.message}</p>
				</div>

				<button
					class="rounded-full p-1 text-current/70 transition hover:bg-white/50 hover:text-current"
					onclick={() => alerts.dismiss(alert.id)}
					aria-label="Dismiss notification"
				>
					<X size={16} />
				</button>
			</div>

			<div class="absolute inset-x-0 bottom-0 h-1 bg-black/5">
				<div
					class={`h-full origin-left ${progressStyles[alert.severity]}`}
					style={`animation: shrink ${alert.remaining}ms linear forwards;`}
				></div>
			</div>
		</div>
	{/each}
</div>

<style>
	@keyframes shrink {
		from {
			transform: scaleX(1);
		}
		to {
			transform: scaleX(0);
		}
	}
</style>
