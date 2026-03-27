<svelte:options runes={true} />

<script module lang="ts">
	export type Option<T = string> = {
		label: string;
		value: T;
	};
</script>

<script lang="ts" generics="T = string">
	import { Spring } from 'svelte/motion';
	import { onMount } from 'svelte';

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

	let containerEl = $state<HTMLElement | null>(null);
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

	function handleTouchStart(e: TouchEvent) {
		if (disabled || !e.touches[0]) return;
		startX = e.touches[0].clientX;
		startY = e.touches[0].clientY;
		startIndex = index;
		dragging = true;
		intentDetermined = false;
		isHorizontalDrag = false;
	}

	function handleTouchMove(e: TouchEvent) {
		if (!dragging || !e.touches[0]) return;

		const dx = e.touches[0].clientX - startX;
		const dy = e.touches[0].clientY - startY;

		if (!intentDetermined) {
			if (Math.abs(dx) < 5 && Math.abs(dy) < 5) return;
			isHorizontalDrag = Math.abs(dx) > Math.abs(dy);
			intentDetermined = true;
		}

		if (!isHorizontalDrag) return;

		e.preventDefault();
		e.stopPropagation();

		const segmentWidth = containerWidth / options.length;
		const next = Math.max(0, Math.min(options.length - 1, startIndex + dx / segmentWidth));
		x.set(next);
	}

	function handleTouchEnd(e: TouchEvent) {
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

	onMount(() => {
		const el = containerEl;
		if (!el) return;

		// passive: false is what makes preventDefault() actually work in a WebView
		el.addEventListener('touchstart', handleTouchStart, { passive: true });
		el.addEventListener('touchmove', handleTouchMove, { passive: false });
		el.addEventListener('touchend', handleTouchEnd, { passive: true });

		return () => {
			el.removeEventListener('touchstart', handleTouchStart);
			el.removeEventListener('touchmove', handleTouchMove);
			el.removeEventListener('touchend', handleTouchEnd);
		};
	});
</script>

<div
	role="radiogroup"
	tabindex="0"
	aria-disabled={disabled}
	class="relative flex min-h-11 overflow-hidden rounded-full border border-slate-200 bg-slate-50 p-1 shadow-sm"
	class:pointer-events-none={disabled}
	bind:this={containerEl}
	bind:clientWidth={containerWidth}
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

	{#each options as opt, i (opt.value)}
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
