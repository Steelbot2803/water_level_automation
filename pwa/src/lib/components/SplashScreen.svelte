<svelte:options runes={true} />

<script lang="ts">
	import logo from '$lib/img/neptune_icon.png';

	// Parent toggles splash visibility.
	let { visible }: { visible: boolean } = $props();
</script>

<div
	class="fixed inset-0 z-[100] flex flex-col items-center justify-center gap-8 bg-gradient-to-b from-cyan-50 via-white to-slate-100 transition-opacity duration-300"
	class:opacity-0={!visible}
	class:pointer-events-none={!visible}
	aria-hidden={!visible}
>
	<div class="flex flex-col items-center gap-5">
		<div
			class="flex h-24 w-24 items-center justify-center rounded-[2rem] shadow-xl shadow-cyan-300/60"
		>
			<img src={logo} alt="Neptune logo" class="h-20 w-20 rounded-[1.6rem]" />
		</div>

		<div class="text-center">
			<h1 class="text-4xl font-semibold tracking-tight text-slate-900 uppercase">Neptune</h1>
			<p class="mt-1 text-sm tracking-[0.25em] text-slate-400 uppercase">Water Flow Automation</p>
		</div>
	</div>

	<!-- CSS-only loading dots. -->
	<div class="flex gap-2">
		{#each [0, 1, 2] as i (i)}
			<div
				class="h-2 w-2 rounded-full bg-cyan-400"
				style="animation: splash-pulse 1.2s ease-in-out {i * 0.2}s infinite both;"
			></div>
		{/each}
	</div>
</div>

<style>
	@keyframes splash-pulse {
		0%,
		80%,
		100% {
			opacity: 0.25;
			transform: scale(0.8);
		}
		40% {
			opacity: 1;
			transform: scale(1);
		}
	}
</style>
