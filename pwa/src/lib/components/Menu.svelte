<script lang="ts">
	import { onDestroy } from 'svelte';
	import { X, Sun, Moon, Monitor, Bell, Activity, RotateCcw } from 'lucide-svelte';
	import { theme, themeIcons, type ThemePreference } from '$lib/stores/theme.js';
	import { notificationPrefs, notificationLabels } from '$lib/stores/notifications.js';
	import { waterSystem } from '$lib/stores/system.js';

	let { open = $bindable(false) }: { open: boolean } = $props();

	let resetConfirming = $state(false);
	let resetTimer: ReturnType<typeof setTimeout> | null = null;
	function handleReset() {
		if (!resetConfirming) {
			// First tap: arm the button with a 3s window
			resetConfirming = true;
			resetTimer = setTimeout(() => {
				resetConfirming = false;
			}, 3000);
			return;
		}

		// Second tap within 3s: execute
		if (resetTimer) clearTimeout(resetTimer);
		resetConfirming = false;
		waterSystem.sendCommand('reset');
		// Give the Arduino ~4s to reboot before reloading the PWA
		setTimeout(() => location.reload(), 4000);
	}

	function close() {
		open = false;
		resetConfirming = false;
		if (resetTimer) clearTimeout(resetTimer);
	}

	function lockPageScroll() {
		if (typeof document === 'undefined') return;

		document.documentElement.style.overflow = 'hidden';
		document.body.style.overflow = 'hidden';
		document.body.style.touchAction = 'none';
	}

	function unlockPageScroll() {
		if (typeof document === 'undefined') return;

		document.documentElement.style.overflow = '';
		document.body.style.overflow = '';
		document.body.style.touchAction = '';
	}

	$effect(() => {
		if (open) {
			lockPageScroll();
			return () => unlockPageScroll();
		}
	});

	onDestroy(() => {
		unlockPageScroll();
	});

	const themeIconComponents: Record<string, typeof Sun> = {
		light: Sun,
		system: Monitor,
		dark: Moon
	};
</script>

<!-- Backdrop -->
<!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
<div
	role="button"
	tabindex={open ? 0 : -1}
	class={`fixed inset-0 z-40 transition-opacity duration-300 ease-out ${
		open ? 'bg-black/30 opacity-100 backdrop-blur-sm' : 'pointer-events-none opacity-0'
	}`}
	class:pointer-events-none={!open}
	onclick={close}
	onkeydown={(e) => e.key == 'Escape' && close()}
></div>

<!-- Drawer -->
<div
	class={`fixed top-0 right-0 z-50 flex h-dvh min-h-0 w-full max-w-xs flex-col overflow-hidden bg-white transition-all duration-300 ease-out ${
		open
			? 'translate-x-0 opacity-100 shadow-2xl'
			: 'pointer-events-none translate-x-[110%] opacity-0 shadow-none'
	}`}
	class:translate-x-0={open}
	role="dialog"
	aria-modal="true"
	aria-label="Settings menu"
>
	<div class="flex items-center justify-between border-b border-slate-200 px-5 py-4">
		<h2 class="text-lg font-semibold tracking-wide uppercase">Settings</h2>
		<button
			onclick={close}
			class="rounded-full p-2 text-slate-500 transition hover:bg-slate-100"
			aria-label="Close menu"
		>
			<X size={20} />
		</button>
	</div>

	<div
		class="min-h-0 flex-1 [touch-action:pan-y] space-y-6 overflow-y-auto overscroll-contain px-5 py-4 [-webkit-overflow-scrolling:touch]"
	>
		<!-- Theme -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">Theme</p>
			<div class="flex gap-2">
				{#each themeIcons as { val, icon: Icon }}
					<button
						onclick={() => theme.set(val as ThemePreference)}
						class="flex flex-1 flex-col items-center gap-1.5 rounded-2xl border px-3 py-3 text-xs font-semibold uppercase transition
							{$theme === val
							? 'border-cyan-500 bg-cyan-50 text-cyan-700'
							: 'border-slate-200 bg-slate-50 text-slate-500 hover:bg-slate-100'}"
					>
						<Icon size={18} />
						{val}
					</button>
				{/each}
			</div>
		</section>

		<!-- Notifications -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">
				Notifications
			</p>
			<div class="space-y-1 overflow-hidden rounded-2xl border border-slate-200 bg-slate-50">
				{#each Object.entries(notificationLabels) as [key, label]}
					<button
						onclick={() => notificationPrefs.toggle(key as keyof typeof $notificationPrefs)}
						class="flex w-full items-center justify-between px-4 py-3 text-sm transition hover:bg-slate-100"
					>
						<span class="text-slate-700">{label}</span>
						<!-- Toggle pill -->
						<div
							class="relative h-6 w-10 rounded-full transition-colors duration-200
								{$notificationPrefs[key as keyof typeof $notificationPrefs] ? 'bg-cyan-500' : 'bg-slate-300'}"
						>
							<div
								class="absolute top-1 h-4 w-4 rounded-full bg-white shadow transition-transform duration-200
									{$notificationPrefs[key as keyof typeof $notificationPrefs] ? 'translate-x-5' : 'translate-x-1'}"
							></div>
						</div>
					</button>
				{/each}
			</div>
		</section>

		<!-- Status output -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">
				Device Status
			</p>
			{#if $waterSystem.device}
				<div class="rounded-2xl border border-slate-200 bg-slate-50 p-4">
					<div class="space-y-1.5 font-mono text-xs text-slate-600">
						<div class="flex justify-between">
							<span class="text-slate-400">mode</span><span>{$waterSystem.device.mode}</span>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">overhead</span><span>{$waterSystem.device.overhead}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">sump</span><span>{$waterSystem.device.sump}</span>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">motor</span><span>{$waterSystem.device.motor}</span>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">borewell</span><span
								>{$waterSystem.device.borewell_status}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">sump pump</span><span
								>{$waterSystem.device.sump_status}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">override</span><span>{$waterSystem.device.override}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">estop</span><span
								>{$waterSystem.device.emergency_stop ?? false}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">wifi</span><span
								>{$waterSystem.wifiConnection.wifiPhase}</span
							>
						</div>
						<div class="flex justify-between">
							<span class="text-slate-400">arduino mqtt</span><span
								>{$waterSystem.arduinoMQTTConnection.mqttPhase}</span
							>
						</div>
					</div>
				</div>
			{:else}
				<p class="text-sm text-slate-400">No device data yet.</p>
			{/if}
		</section>

		<!-- Reset -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">System</p>
			<button
				onclick={handleReset}
				class="flex w-full items-center justify-center gap-2 rounded-2xl border px-4 py-3 text-sm font-semibold uppercase transition
					{resetConfirming
					? 'animate-pulse border-rose-400 bg-rose-500 text-white'
					: 'border-slate-200 bg-slate-50 text-slate-700 hover:bg-slate-100'}"
			>
				<RotateCcw size={16} />
				{resetConfirming ? 'Tap again to confirm reset' : 'Reset Arduino & App'}
			</button>
			{#if resetConfirming}
				<p class="mt-2 text-center text-xs text-rose-500">
					Resets the Arduino and reloads the app. This takes ~4 seconds.
				</p>
			{/if}
		</section>
	</div>
</div>
