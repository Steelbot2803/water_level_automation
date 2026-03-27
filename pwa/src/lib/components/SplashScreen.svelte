<svelte:options runes={true} />

<script lang="ts">
	import logo from '$lib/img/neptune_icon.png';
	import { Dot } from 'lucide-svelte';

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
		<div class="relative h-36 w-36">
			<div class="absolute inset-0 animate-pulse shadow-lg shadow-blue-500 rounded-[1.4rem]"></div>
			<img src={logo} alt="Neptune logo" class="absolute inset-0 h-36 w-36" />
		</div>

		<div class="text-center">
			<h1 class="text-4xl font-semibold tracking-tight text-slate-900 uppercase">Neptune</h1>
			<p class="mt-1 text-sm tracking-[0.25em] text-slate-400 uppercase">Water Flow Automation</p>
		</div>
	</div>
</div>
