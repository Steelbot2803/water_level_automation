<svelte:options runes={true} />

<script lang="ts">
	import { onDestroy } from 'svelte';
	import { X, RotateCw, RotateCcw, LogOut, QrCode } from 'lucide-svelte';
	import { theme, themeIcons, type ThemePreference } from '$lib/stores/theme.js';
	import { notificationPrefs, notificationLabels } from '$lib/stores/notifications.js';
	import {
		arduinoMqttConnectionState,
		deviceTelemetry,
		waterSystem,
		wifiConnectionState
	} from '$lib/stores/system.js';
	import { encodeBrokerSettingsAsQR } from '$lib/control.js';
	import QRCode from 'qrcode';

	let { open = $bindable(false) }: { open: boolean } = $props();

	let status = $derived({
		Mode: $deviceTelemetry?.mode,
		'Overhead Tank': $deviceTelemetry?.overhead,
		'Sump Tank': $deviceTelemetry?.sump,
		Motor: $deviceTelemetry?.motor,
		'Borewell Pump': $deviceTelemetry?.borewell_status,
		'Sump Pump': $deviceTelemetry?.sump_status,
		Override: $deviceTelemetry?.override,
		'Emergency Stop': $deviceTelemetry?.emergency_stop ?? false,
		'Arduino WiFi': $wifiConnectionState.wifiPhase,
		'Arduino MQTT': $arduinoMqttConnectionState.mqttPhase
	});

	let restarting = $state(false);
	let restartProgress = $state(0);
	let restartInterval: ReturnType<typeof setInterval> | null = null;
	let resetConfirming = $state(false);
	let stateResetConfirming = $state(false);
	let disconnectConfirming = $state(false);
	let resetTimer: ReturnType<typeof setTimeout> | null = null;
	let stateResetTimer: ReturnType<typeof setTimeout> | null = null;
	let disconnectTimer: ReturnType<typeof setTimeout> | null = null;
	let qrDataUrl = $state<string | null>(null);
	let qrOpen = $state(false);

	function handleReset() {
		if (!resetConfirming) {
			resetConfirming = true;
			resetTimer = setTimeout(() => (resetConfirming = false), 3000);
			return;
		}
		if (resetTimer) clearTimeout(resetTimer);
		resetConfirming = false;
		waterSystem.sendCommand('reset');
		restarting = true;
		restartProgress = 0;
		const DURATION = 10000;
		const TICK = 100;
		restartInterval = setInterval(() => {
			restartProgress += (TICK / DURATION) * 100;
			if (restartProgress >= 100) {
				restartProgress = 100;
				if (restartInterval) clearInterval(restartInterval);
				location.reload();
			}
		}, TICK);
	}

	function handleStateReset() {
		if (!stateResetConfirming) {
			stateResetConfirming = true;
			stateResetTimer = setTimeout(() => (stateResetConfirming = false), 3000);
			return;
		}
		if (stateResetTimer) clearTimeout(stateResetTimer);
		stateResetConfirming = false;
		waterSystem.sendCommand('reset state');
	}

	function handleDisconnect() {
		if (!disconnectConfirming) {
			disconnectConfirming = true;
			disconnectTimer = setTimeout(() => (disconnectConfirming = false), 3000);
			return;
		}
		if (disconnectTimer) clearTimeout(disconnectTimer);
		disconnectConfirming = false;
		close();
		// Clears local credentials and returns to login.
		waterSystem.clearCredentials();
	}

	function close() {
		open = false;
		resetConfirming = false;
		stateResetConfirming = false;
		disconnectConfirming = false;
		if (resetTimer) clearTimeout(resetTimer);
		if (stateResetTimer) clearTimeout(stateResetTimer);
		if (disconnectTimer) clearTimeout(disconnectTimer);
		if (restartInterval) clearInterval(restartInterval);
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

	async function openQr() {
		const encoded = encodeBrokerSettingsAsQR($waterSystem.settings);
		qrDataUrl = await QRCode.toDataURL(encoded, { width: 200, margin: 2 });
		qrOpen = true;
	}

	function closeQr() {
		qrOpen = false;
		qrDataUrl = null;
	}

	$effect(() => {
		if (open) {
			lockPageScroll();
			return () => unlockPageScroll();
		}
	});

	onDestroy(() => {
		unlockPageScroll();
		if (restartInterval) clearInterval(restartInterval);
	});
</script>

<!-- Backdrop -->
<div
	role="button"
	tabindex={open ? 0 : -1}
	class={`fixed inset-0 z-40 transition-opacity duration-300 ease-out ${open ? 'bg-black/30 opacity-100 backdrop-blur-sm' : 'pointer-events-none opacity-0'}`}
	class:pointer-events-none={!open}
	onclick={close}
	onkeydown={(e) => e.key === 'Escape' && close()}
></div>

<!-- Drawer -->
<div
	class={`fixed top-0 right-0 z-50 flex h-dvh min-h-0 w-full max-w-xs flex-col overflow-hidden bg-white transition-all duration-300 ease-out ${open ? 'translate-x-0 opacity-100 shadow-2xl' : 'pointer-events-none translate-x-[110%] opacity-0 shadow-none'}`}
	role="dialog"
	aria-modal="true"
	aria-label="Settings menu"
>
	<div class="flex items-center justify-between border-b border-slate-200 px-5 py-4">
		<h2 class="text-lg font-semibold tracking-wide uppercase">Settings</h2>
		<button
			onclick={close}
			class="flex h-11 w-11 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition hover:bg-slate-100 active:scale-[0.98]"
			aria-label="Close menu"
		>
			<X size={20} />
		</button>
	</div>

	<div
		class="min-h-0 flex-1 [touch-action:pan-y] space-y-6 overflow-y-auto overscroll-contain px-5 py-4 [-webkit-overflow-scrolling:touch]"
	>
		<!-- QR Code -->
		<section>
			{#if !qrOpen}
				<button
					onclick={() => openQr()}
					class="mb-2 flex min-h-10 w-full items-center justify-center gap-2 rounded-full border border-slate-200 bg-slate-50 px-4 py-2 text-base font-semibold tracking-[0.14em] text-slate-700 uppercase shadow-sm transition hover:bg-slate-100 active:scale-[0.985]"
				>
					<QrCode size={18} /> Credentials QR</button
				>
			{:else}
				<button
					onclick={() => closeQr()}
					class="mb-2 flex min-h-10 w-full items-center justify-center gap-2 rounded-full border border-slate-200 bg-slate-50 px-4 py-2 text-base font-semibold tracking-[0.14em] text-slate-700 uppercase shadow-sm transition hover:bg-slate-100 active:scale-[0.985]"
				>
					<X size={18} /> Close
				</button>
			{/if}

			{#if qrDataUrl}
				<img
					src={qrDataUrl}
					alt="Credentials QR code"
					class="mx-auto mt-2 h-2/3 w-2/3 rounded-xl"
				/>
			{/if}
		</section>
		<!-- Theme -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">Theme</p>
			<div class="flex gap-2">
				{#each themeIcons as { val, icon: Icon } (val)}
					<button
						onclick={() => theme.set(val as ThemePreference)}
						class="flex min-h-14 flex-1 flex-col items-center justify-center gap-1.5 rounded-[1.4rem] border px-3 py-3 text-xs font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985]"
						class:border-cyan-300={$theme === val}
						class:bg-cyan-50={$theme === val}
						class:text-cyan-700={$theme === val}
						class:border-slate-200={$theme !== val}
						class:bg-slate-50={$theme !== val}
						class:text-slate-500={$theme !== val}
						class:hover:bg-slate-100={$theme !== val}
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
			<div class="space-y-2">
				{#each Object.entries(notificationLabels) as [key, label] (key)}
					<button
						onclick={() => notificationPrefs.toggle(key as keyof typeof $notificationPrefs)}
						class="flex min-h-14 w-full items-center justify-between rounded-[1.4rem] border border-slate-200 bg-slate-50 px-4 py-3 text-sm shadow-sm transition hover:bg-slate-100 active:scale-[0.985]"
					>
						<span class="pr-4 text-left font-medium text-slate-700">{label}</span>
						<div
							class="relative h-7 w-12 shrink-0 rounded-full border transition-colors duration-200 {$notificationPrefs[
								key as keyof typeof $notificationPrefs
							]
								? 'border-cyan-300 bg-cyan-100'
								: 'border-slate-200 bg-white'}"
						>
							<div
								class="absolute top-[3px] h-5 w-5 rounded-full shadow-sm transition-transform duration-200 {$notificationPrefs[
									key as keyof typeof $notificationPrefs
								]
									? 'translate-x-6 bg-cyan-600'
									: 'translate-x-[3px] bg-slate-400'}"
							></div>
						</div>
					</button>
				{/each}
			</div>
		</section>

		<!-- Device status -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">
				Device Status
			</p>
			{#if $deviceTelemetry}
				<div
					class="space-y-1.5 rounded-2xl border border-slate-200 bg-slate-50 p-4 font-mono text-xs text-slate-600"
				>
					{#each Object.entries(status) as [label, value] (label)}
						<div class="flex items-center justify-between">
							<span class="text-slate-400 uppercase">{label}</span>
							<span>{value}</span>
						</div>
					{/each}
				</div>
			{:else}
				<div
					class="flex items-center justify-center rounded-2xl border border-slate-200 bg-slate-50 p-4"
				>
					<p class="font-mono text-xs text-slate-400">No device data yet.</p>
				</div>
			{/if}
		</section>

		<!-- System -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">System</p>

			<button
				onclick={handleStateReset}
				class="mb-2 flex min-h-10 w-full items-center justify-center gap-2 rounded-full border px-4 py-2 text-base font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985] {stateResetConfirming
					? 'animate-pulse border-amber-300 bg-amber-50 text-amber-700'
					: 'border-slate-200 bg-slate-50 text-slate-700 hover:bg-slate-100'}"
			>
				<RotateCw size={20} />
				{stateResetConfirming ? 'Confirm Reset' : 'State Reset'}
			</button>

			{#if restarting}
				<div class="mb-2 overflow-hidden rounded-full border border-rose-300 bg-rose-50">
					<div class="flex min-h-10 items-center justify-center gap-2 px-4 py-2">
						<RotateCcw size={20} class="animate-spin text-rose-700 [animation-direction:reverse]" />
						<span class="text-base font-semibold tracking-[0.14em] text-rose-700 uppercase"
							>Restarting…</span
						>
					</div>
					<div class="mx-4 mb-3 h-1.5 overflow-hidden rounded-full bg-rose-100">
						<div
							class="h-full rounded-full bg-rose-500 transition-[width] duration-100 ease-linear"
							style="width: {restartProgress}%"
						></div>
					</div>
				</div>
			{:else}
				<button
					onclick={handleReset}
					class="mb-2 flex min-h-10 w-full items-center justify-center gap-2 rounded-full border px-4 py-2 text-base font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985] {resetConfirming
						? 'animate-pulse border-rose-300 bg-rose-50 text-rose-700'
						: 'border-slate-200 bg-slate-50 text-slate-700 hover:bg-slate-100'}"
				>
					<RotateCcw size={20} />
					{resetConfirming ? 'Confirm Restart' : 'System Restart'}
				</button>
			{/if}

			<!-- Two-step confirm avoids accidental disconnect. -->
			<button
				onclick={handleDisconnect}
				class="mb-2 flex min-h-10 w-full items-center justify-center gap-2 rounded-full border px-4 py-2 text-base font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985] {disconnectConfirming
					? 'animate-pulse border-rose-300 bg-rose-50 text-rose-700'
					: 'border-slate-200 bg-slate-50 text-slate-700 hover:bg-slate-100'}"
			>
				<LogOut size={20} />
				{disconnectConfirming ? 'Confirm Disconnect' : 'Disconnect'}
			</button>
		</section>
	</div>
</div>
