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
		disabled = false,
		change
	}: {
		options: Option<T>[];
		value: T;
		color?: string;
		disabled?: boolean;
		change?: (value: T, index: number) => void;
	} = $props();

	// internal state
	let containerWidth = $state(0);
	let dragging = $state(false);
	let startX = $state(0);
	let startIndex = $state(0);

	// derived index
	let index = $derived(
		Math.max(
			0,
			options.findIndex((o) => o.value === value)
		)
	);

	// spring (index-based position)
	const x = new Spring(0, {
		stiffness: 0.25,
		damping: 0.6
	});

	// sync when external value changes
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
		if (disabled) return;
		if (!e.touches[0]) return;
		startX = e.touches[0].clientX;
		startIndex = index;
		dragging = true;
	}

	function ontouchmove(e: TouchEvent) {
		if (!dragging) return;

		if (!e.touches[0]) return;

		const dx = e.touches[0].clientX - startX;
		const segmentWidth = containerWidth / options.length;

		let next = startIndex + dx / segmentWidth;
		next = Math.max(0, Math.min(options.length - 1, next));

		x.set(next);
	}

	function ontouchend(e: TouchEvent) {
		if (!dragging) return;

		if (!e.changedTouches[0]) return;

		const dx = e.changedTouches[0].clientX - startX;
		const segmentWidth = containerWidth / options.length;

		const moved = startIndex + dx / segmentWidth;
		const snapped = Math.round(moved);

		setIndex(snapped);
		dragging = false;
	}
</script>

<div
	role="radiogroup"
	tabindex="0"
	aria-disabled={disabled}
	class="relative flex h-11 touch-pan-x overflow-hidden rounded-2xl bg-slate-200 p-1"
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
	<!-- Slider -->
	<div
		class={`absolute top-1 bottom-1 rounded-xl ${color} shadow-sm transition-colors duration-200`}
		style="
			width: calc((100% - 8px) / {options.length});
			transform: translateX(calc({x.current} * 100%));
		"
	></div>

	<!-- Options -->
	{#each options as opt, i}
		<button
			role="radio"
			aria-checked={i === index}
			tabindex={i === index ? 0 : -1}
			class="relative z-10 flex-1 rounded-xl text-sm font-semibold uppercase transition-colors duration-200 disabled:opacity-40"
			class:text-white={i === Math.round(x.current)}
			class:text-slate-500={i !== Math.round(x.current)}
			{disabled}
			onclick={() => setIndex(i)}
		>
			{opt.label}
		</button>
	{/each}
</div>

<style>
	.touch-pan-x {
		touch-action: pan-x;
	}
</style>
