<script lang="ts">
	import './layout.css';
	import AlertMonitor from '$lib/components/AlertMonitor.svelte';
	import Toasts from '$lib/components/Toasts.svelte';

	const { children } = $props();
</script>

<svelte:options runes={true} />

<AlertMonitor />
{@render children()}
<Toasts />
