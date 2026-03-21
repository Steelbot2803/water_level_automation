<svelte:options runes={true} />

<script lang="ts">
	import './layout.css';
	import AlertMonitor from '$lib/components/AlertMonitor.svelte';
	import Toasts from '$lib/components/Toasts.svelte';

	const { children } = $props();
</script>

<AlertMonitor />
{@render children()}
<Toasts />
