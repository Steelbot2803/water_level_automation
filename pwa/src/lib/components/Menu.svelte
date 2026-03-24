<svelte:options runes={true} />

<script lang="ts">
	import { onDestroy } from 'svelte';
	import { cubicOut } from 'svelte/easing';
	import { browser } from '$app/environment';
	import {
		X,
		RotateCw,
		RotateCcw,
		ScanQrCode,
		QrCode,
		Eye,
		EyeOff,
		CircleCheck
	} from 'lucide-svelte';
	import QRCode from 'qrcode';
	import { theme, themeIcons, type ThemePreference } from '$lib/stores/theme.js';
	import { notificationPrefs, notificationLabels } from '$lib/stores/notifications.js';
	import { waterSystem } from '$lib/stores/system.js';
	import { encodeBrokerSettingsAsQR } from '$lib/control.js';
	import type { BrokerSettings } from '$lib/types.js';
	import QrScanner from '$lib/components/QrScanner.svelte';

	let { open = $bindable(false) }: { open: boolean } = $props();

	// ── draft settings ────────────────────────────────────────────────────────
	let draftSettings = $state<BrokerSettings>({ ...$waterSystem.settings });
	let showPassword = $state(false);
	let saveSuccess = $state(false);

	$effect(() => {
		if (open) draftSettings = { ...$waterSystem.settings };
	});

	function saveConnection(settings: BrokerSettings = draftSettings) {
		waterSystem.updateSettings({ ...settings });
		saveSuccess = true;
		setTimeout(() => (saveSuccess = false), 2000);
	}

	// ── QR panel ──────────────────────────────────────────────────────────────
	let activePanel = $state<'scan' | 'qr' | null>(null);
	let qrDataUrl = $state<string | null>(null);

	// Called by QrScanner when a valid Neptune QR is decoded.
	// Immediately merges credentials, saves & connects — no user tap required.
	// Then shows the success overlay before closing the scanner.
	function handleQrResult(partial: Partial<BrokerSettings>) {
		const merged: BrokerSettings = { ...draftSettings, ...partial };
		draftSettings = merged;
		activePanel = null;
		saveConnection(merged);
		showSuccessOverlay();
	}

	async function openQrPanel() {
		const encoded = encodeBrokerSettingsAsQR($waterSystem.settings);
		qrDataUrl = await QRCode.toDataURL(encoded, { width: 220, margin: 2 });
		activePanel = 'qr';
	}

	function closePanel() {
		activePanel = null;
		qrDataUrl = null;
	}

	// ── success overlay ───────────────────────────────────────────────────────
	// Shown after a successful QR scan+connect. Displays a circular checkmark
	// that draws itself via SVG stroke animation, then auto-dismisses.
	let showSuccess = $state(false);
	let successDismissId: ReturnType<typeof setTimeout> | null = null;

	function showSuccessOverlay() {
		showSuccess = true;
		if (successDismissId) clearTimeout(successDismissId);
		// 2800 ms: 300 ms fade-in + 1800 ms hold + 700 ms fade-out (handled by CSS)
		successDismissId = setTimeout(() => {
			showSuccess = false;
			successDismissId = null;
		}, 2800);
	}

	// ── device status ─────────────────────────────────────────────────────────
	let status = $derived({
		Mode: $waterSystem.device?.mode,
		'Overhead Tank': $waterSystem.device?.overhead,
		'Sump Tank': $waterSystem.device?.sump,
		Motor: $waterSystem.device?.motor,
		'Borewell Pump': $waterSystem.device?.borewell_status,
		'Sump Pump': $waterSystem.device?.sump_status,
		Override: $waterSystem.device?.override,
		'Emergency Stop': $waterSystem.device?.emergency_stop ?? false,
		'Arduino WiFi': $waterSystem.wifiConnection.wifiPhase,
		'Arduino MQTT': $waterSystem.arduinoMQTTConnection.mqttPhase
	});

	// ── restart / reset ───────────────────────────────────────────────────────
	let restarting = $state(false);
	let restartProgress = $state(0);
	let restartInterval: ReturnType<typeof setInterval> | null = null;
	let resetConfirming = $state(false);
	let stateResetConfirming = $state(false);
	let resetTimer: ReturnType<typeof setTimeout> | null = null;
	let stateResetTimer: ReturnType<typeof setTimeout> | null = null;

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
		const DURATION = 15000;
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

	function close() {
		open = false;
		resetConfirming = false;
		stateResetConfirming = false;
		activePanel = null;
		qrDataUrl = null;
		if (resetTimer) clearTimeout(resetTimer);
		if (stateResetTimer) clearTimeout(stateResetTimer);
		if (restartInterval) clearInterval(restartInterval);
		if (successDismissId) {
			clearTimeout(successDismissId);
			showSuccess = false;
		}
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
		if (restartInterval) clearInterval(restartInterval);
		if (successDismissId) clearTimeout(successDismissId);
	});

	// ── transitions ───────────────────────────────────────────────────────────
	// Incoming element falls from above into place.
	// Outgoing element exits upward via position:absolute (leaves flow immediately)
	// so only the incoming element drives container height throughout.

	function panelIn(_node: Element) {
		if (!browser) return { duration: 0 };
		return {
			duration: 280,
			easing: cubicOut,
			css: (t: number) => `opacity: ${t}; transform: translateY(${(1 - t) * -100}%);`
		};
	}

	function panelOut(_node: Element) {
		if (!browser) return { duration: 0 };
		return {
			duration: 220,
			easing: cubicOut,
			css: (t: number) =>
				`position: absolute; inset: 0; opacity: ${t * t}; transform: translateY(${(1 - t) * -30}%);`
		};
	}
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
	onkeydown={(e) => e.key === 'Escape' && close()}
></div>

<!-- Drawer -->
<div
	class={`fixed top-0 right-0 z-50 flex h-dvh min-h-0 w-full max-w-xs flex-col overflow-hidden bg-white transition-all duration-300 ease-out ${
		open
			? 'translate-x-0 opacity-100 shadow-2xl'
			: 'pointer-events-none translate-x-[110%] opacity-0 shadow-none'
	}`}
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
		<!-- Connection -->
		<section>
			<div class="mb-3 flex items-center justify-between">
				<p class="text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">Connection</p>
				{#if !activePanel}
					<div class="flex items-center gap-1.5">
						<button
							onclick={() => (activePanel = 'scan')}
							class="flex h-8 w-8 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition hover:bg-slate-100 active:scale-[0.98]"
							aria-label="Scan QR code"
							title="Scan QR code"
						>
							<ScanQrCode size={16} />
						</button>
						<button
							onclick={openQrPanel}
							class="flex h-8 w-8 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition hover:bg-slate-100 active:scale-[0.98]"
							aria-label="Show QR code"
							title="Show QR code"
						>
							<QrCode size={16} />
						</button>
					</div>
				{/if}
			</div>

			<div class="relative overflow-hidden">
				{#if activePanel === 'scan'}
					<div
						in:panelIn
						out:panelOut
						class="relative overflow-hidden rounded-2xl border border-slate-200 bg-black"
						style="height: 220px;"
					>
						<QrScanner inline onResult={handleQrResult} onClose={closePanel} />
						<button
							onclick={closePanel}
							class="absolute top-2 right-2 flex h-7 w-7 items-center justify-center rounded-full border border-slate-200 bg-black text-white backdrop-blur-sm transition hover:bg-black/70 active:scale-[0.97]"
							aria-label="Close scanner"
						>
							<X size={14} />
						</button>
					</div>
				{:else if activePanel === 'qr'}
					<div
						in:panelIn
						out:panelOut
						class="relative flex flex-col items-center gap-2 rounded-2xl border border-slate-200 bg-slate-50 p-4"
					>
						<button
							onclick={closePanel}
							class="absolute top-2 right-2 flex h-7 w-7 items-center justify-center rounded-full border border-slate-200 bg-black text-white backdrop-blur-sm transition hover:bg-black/70 active:scale-[0.97]"
							aria-label="Close QR code"
						>
							<X size={14} />
						</button>
						{#if qrDataUrl}
							<img src={qrDataUrl} alt="Connection QR code" class="mt-4 h-32 w-32 rounded-xl" />
							<p class="text-xs text-slate-400 uppercase">Scan with another device</p>
						{/if}
					</div>
				{:else}
					<div in:panelIn out:panelOut>
						<form
							onsubmit={(e) => {
								e.preventDefault();
								saveConnection();
							}}
							autocomplete="on"
							class="space-y-2"
						>
							<input
								type="text"
								placeholder="Username"
								bind:value={draftSettings.username}
								autocomplete="username"
								class="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-800 placeholder:text-slate-400 focus:border-cyan-300 focus:outline-none"
							/>
							<div class="relative">
								{#if showPassword}
									<input
										type="text"
										placeholder="Password"
										bind:value={draftSettings.password}
										autocomplete="current-password"
										class="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 pr-12 text-sm text-slate-800 placeholder:text-slate-400 focus:border-cyan-300 focus:outline-none"
									/>
								{:else}
									<input
										type="password"
										placeholder="Password"
										bind:value={draftSettings.password}
										autocomplete="current-password"
										class="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 pr-12 text-sm text-slate-800 placeholder:text-slate-400 focus:border-cyan-300 focus:outline-none"
									/>
								{/if}
								<button
									type="button"
									onclick={() => (showPassword = !showPassword)}
									class="absolute top-1/2 right-4 -translate-y-1/2 text-slate-400 transition hover:text-slate-600"
									aria-label={showPassword ? 'Hide password' : 'Show password'}
								>
									{#if showPassword}
										<EyeOff size={16} />
									{:else}
										<Eye size={16} />
									{/if}
								</button>
							</div>
							<button
								onclick={() => saveConnection()}
								class={`mt-2 flex min-h-11 w-full items-center justify-center rounded-full border px-4 py-2 text-sm font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.98] ${
									saveSuccess
										? 'border-emerald-300 bg-emerald-50 text-emerald-700'
										: 'border-cyan-300 bg-cyan-50 text-cyan-700 hover:bg-cyan-100'
								}`}
							>
								{#if saveSuccess}
									<CircleCheck size={16} class="mr-2" /> Saved!
								{:else}
									Save & Connect
								{/if}
							</button>
						</form>
					</div>
				{/if}
			</div>
		</section>

		<!-- Theme -->
		<section>
			<p class="mb-3 text-xs font-semibold tracking-[0.2em] text-slate-400 uppercase">Theme</p>
			<div class="flex gap-2">
				{#each themeIcons as { val, icon: Icon }}
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
				{#each Object.entries(notificationLabels) as [key, label]}
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
			{#if $waterSystem.device}
				<div
					class="space-y-1.5 rounded-2xl border border-slate-200 bg-slate-50 p-4 font-mono text-xs text-slate-600"
				>
					{#each Object.entries(status) as [label, value]}
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
				<div class="overflow-hidden rounded-full border border-rose-300 bg-rose-50">
					<div class="flex min-h-10 items-center justify-center gap-2 px-4 py-2">
						<RotateCcw size={20} class="animate-spin text-rose-700 [animation-direction:reverse]" />
						<span class="text-base font-semibold tracking-[0.14em] text-rose-700 uppercase">
							Restarting...
						</span>
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
					class="flex min-h-10 w-full items-center justify-center gap-2 rounded-full border px-4 py-2 text-base font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.985] {resetConfirming
						? 'animate-pulse border-rose-300 bg-rose-50 text-rose-700'
						: 'border-slate-200 bg-slate-50 text-slate-700 hover:bg-slate-100'}"
				>
					<RotateCcw size={20} />
					{resetConfirming ? 'Confirm Restart' : 'System Restart'}
				</button>
			{/if}
		</section>
	</div>

	<!--
		Success overlay — sits inside the drawer (z-10) so it covers the content
		area but not the header, giving spatial context that something happened here.
		Uses CSS keyframes for the checkmark draw and the overall fade.
	-->
	{#if showSuccess}
		<div
			class="success-overlay pointer-events-none absolute inset-0 z-10 flex flex-col items-center justify-center gap-4 bg-white/95 backdrop-blur-sm"
		>
			<div
				class="flex h-20 w-20 items-center justify-center rounded-full border-2 border-emerald-200 bg-emerald-50 shadow-sm"
			>
				<!--
					SVG checkmark with stroke-dashoffset animation.
					stroke-dasharray = path length (~52px for this viewBox).
					On mount the dash starts fully offset (invisible), then
					transitions to 0 (fully drawn) over 400ms.
					The surrounding div handles the scale-in independently.
				-->
				<svg viewBox="0 0 52 52" class="success-check h-10 w-10" fill="none">
					<polyline
						points="10,28 22,40 42,16"
						stroke="#059669"
						stroke-width="4"
						stroke-linecap="round"
						stroke-linejoin="round"
					/>
				</svg>
			</div>
			<div class="text-center">
				<p class="text-sm font-semibold tracking-[0.12em] text-emerald-700 uppercase">Connected</p>
				<p class="mt-0.5 text-xs text-slate-400">Credentials saved</p>
			</div>
		</div>
	{/if}
</div>

<style>
	.success-overlay {
		animation: overlay-fade 2.8s ease forwards;
	}

	/*
		Fade in quickly, hold, then fade out.
		Keyframe percentages are time fractions of the total 2800ms duration:
		  0–11%  = 0–300ms   fade in
		  11–75% = 300–2100ms hold
		  75–100% = 2100–2800ms fade out
	*/
	@keyframes overlay-fade {
		0% {
			opacity: 0;
		}
		11% {
			opacity: 0.66;
		}
		75% {
			opacity: 0.66;
		}
		100% {
			opacity: 0;
		}
	}

	/*
		Draws the checkmark stroke from invisible to fully visible.
		stroke-dasharray matches the approximate path length of the polyline.
	*/
	.success-check polyline {
		stroke-dasharray: 52;
		stroke-dashoffset: 52;
		animation: draw-check 0.4s cubic-bezier(0.33, 1, 0.68, 1) 0.15s forwards;
	}

	/* Scale the circle in with a slight spring overshoot */
	.success-overlay > div:first-child {
		animation: pop-in 0.35s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
		transform: scale(0.6);
	}

	@keyframes draw-check {
		to {
			stroke-dashoffset: 0;
		}
	}

	@keyframes pop-in {
		to {
			transform: scale(1);
		}
	}
</style>
