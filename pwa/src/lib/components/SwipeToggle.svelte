<svelte:options runes={true} />

<script module lang="ts">
	export type Option<T = string> = {
		label: string;
		value: T;
	};
</script>

<script lang="ts" generics="T = string">
	import { Spring } from 'svelte/motion';

	let {
		options = [],
		value,
		color,
		textColor,
		disabled = false,
		change
	}: {
		options: Option<T>[];
		value: T;
		color?: string;
		textColor?: string;
		disabled?: boolean;
		change?: (value: T, index: number) => void;
	} = $props();

	let containerWidth = $state(0);
	let dragging = $state(false);
	let startX = $state(0);
	let startY = $state(0);
	let startIndex = $state(0);
	let intentDetermined = $state(false);
	let isHorizontalDrag = $state(false);

	let index = $derived(
		Math.max(
			0,
			options.findIndex((o) => o.value === value)
		)
	);

	const x = new Spring(0, { stiffness: 0.25, damping: 0.6 });

	$effect(() => {
		if (!dragging) x.set(index);
	});

	function setIndex(i: number) {
		if (disabled) return;
		const clamped = Math.max(0, Math.min(options.length - 1, i));
		const selected = options[clamped];
		if (!selected || selected.value === value) return;
		change?.(selected.value, clamped);
		x.set(clamped);
	}

	function ontouchstart(e: TouchEvent) {
		if (disabled || !e.touches[0]) return;
		startX = e.touches[0].clientX;
		startY = e.touches[0].clientY;
		startIndex = index;
		dragging = true;
		intentDetermined = false;
		isHorizontalDrag = false;
	}

	function ontouchmove(e: TouchEvent) {
		if (!dragging || !e.touches[0]) return;

		const dx = e.touches[0].clientX - startX;
		const dy = e.touches[0].clientY - startY;

		if (!intentDetermined) {
			// Need at least 5px of movement before we can tell which direction
			if (Math.abs(dx) < 5 && Math.abs(dy) < 5) return;
			isHorizontalDrag = Math.abs(dx) > Math.abs(dy);
			intentDetermined = true;
		}

		if (!isHorizontalDrag) return;

		// Only block page scroll once we know the user is swiping horizontally
		e.preventDefault();

		const segmentWidth = containerWidth / options.length;
		const next = Math.max(0, Math.min(options.length - 1, startIndex + dx / segmentWidth));
		x.set(next);
	}

	function ontouchend(e: TouchEvent) {
		if (!dragging || !e.changedTouches[0]) return;

		if (isHorizontalDrag) {
			const dx = e.changedTouches[0].clientX - startX;
			const segmentWidth = containerWidth / options.length;
			setIndex(Math.round(startIndex + dx / segmentWidth));
		}

		dragging = false;
		intentDetermined = false;
		isHorizontalDrag = false;
	}
</script>

<div
	role="radiogroup"
	tabindex="0"
	aria-disabled={disabled}
	class="relative flex min-h-11 overflow-hidden rounded-full border border-slate-200 bg-slate-50 p-1 shadow-sm"
	class:pointer-events-none={disabled}
	bind:clientWidth={containerWidth}
	{ontouchstart}
	{ontouchmove}
	{ontouchend}
	onkeydown={(e) => {
		if (disabled) return;
		if (e.key === 'ArrowRight') setIndex(index + 1);
		if (e.key === 'ArrowLeft') setIndex(index - 1);
	}}
>
	<div
		class={`absolute top-1 bottom-1 rounded-full ${color} shadow-sm transition-colors duration-200`}
		style="
			width: calc((100% - 8px) / {options.length});
			transform: translateX(calc({x.current} * 100%));
		"
	></div>

	{#each options as opt, i}
		<button
			role="radio"
			aria-checked={i === index}
			tabindex={i === index ? 0 : -1}
			class={`${i === Math.round(x.current) ? textColor : 'text-slate-500'} text-l relative z-10 flex min-h-9 flex-1 items-center justify-center rounded-full px-3 py-2 font-semibold tracking-[0.14em] uppercase transition-all duration-200 active:scale-[0.985] disabled:pointer-events-none disabled:opacity-40`}
			{disabled}
			onclick={() => setIndex(i)}
		>
			{opt.label}
		</button>
	{/each}
</div>

<style>
	/* Removed — touch-action: pan-x is no longer used */
</style>
