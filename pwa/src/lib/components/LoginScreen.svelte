<svelte:options runes={true} />

<script lang="ts">
	import { LoaderCircle, Wifi, WifiOff, ScanQrCode, Eye, EyeOff } from 'lucide-svelte';
	import { waterSystem } from '$lib/stores/system.js';
	import type { BrokerSettings } from '$lib/types.js';
	import QrScanner from '$lib/components/QrScanner.svelte';
	import logo from '$lib/img/neptune_icon.png';

	let username = $state('');
	let password = $state('');
	let showPassword = $state(false);
	let scannerOpen = $state(false);

	const phase = $derived($waterSystem.mqttConnection.mqttPhase);
	const lastError = $derived($waterSystem.mqttConnection.lastError);

	const isConnecting = $derived(phase === 'connecting' || phase === 'reconnecting');
	const isError = $derived(phase === 'error');

	function handleSubmit() {
		if (!username && !password) return;
		waterSystem.updateSettings({
			...$waterSystem.settings,
			username,
			password
		});
	}

	function handleQrResult(partial: Partial<BrokerSettings>) {
		if (partial.username !== undefined) username = partial.username;
		if (partial.password !== undefined) password = partial.password;
		scannerOpen = false;
		waterSystem.updateSettings({
			...$waterSystem.settings,
			username,
			password
		});
	}
</script>

<div
	class="flex min-h-dvh flex-col items-center justify-center bg-gradient-to-b from-cyan-50 via-white to-slate-100 px-6"
>
	<!-- Brand mark -->
	<div class="mb-10 flex flex-col items-center gap-3">
		<div
			class="flex h-20 w-20 items-center justify-center rounded-[1.4rem] shadow-lg shadow-blue-500"
		>
			<img src={logo} alt="Neptune logo" />
		</div>
		<div class="text-center">
			<h1 class="text-3xl font-semibold tracking-tight text-slate-900 uppercase">Neptune</h1>
			<p class="mt-1 text-sm tracking-wide text-slate-400 uppercase">Water Flow Automation</p>
		</div>
	</div>

	<!-- Card -->
	<div class="w-full max-w-sm rounded-[2rem] border border-slate-200 bg-white p-6 shadow-sm">
		<div class="mb-5 flex items-center justify-between">
			<p class="text-sm font-semibold tracking-[0.15em] text-slate-500 uppercase">
				Broker Credentials
			</p>
			<div class="flex items-center gap-1.5">
				<button
					onclick={() => (scannerOpen = !scannerOpen)}
					class="flex h-8 w-8 items-center justify-center rounded-full border border-slate-200 bg-slate-50 text-slate-500 shadow-sm transition hover:bg-slate-100 active:scale-[0.98]"
					aria-label="Scan QR code"
					title="Scan QR"
				>
					<ScanQrCode size={15} />
				</button>
			</div>
		</div>

		{#if scannerOpen}
			<!-- QR scanner replaces the form temporarily -->
			<div
				class="overflow-hidden rounded-2xl border-2 border-slate-200 bg-black"
				style="height: 320px;"
			>
				<QrScanner inline onResult={handleQrResult} onClose={() => (scannerOpen = false)} />
			</div>
		{:else}
			<form
				onsubmit={(event) => {
					event.preventDefault();
					handleSubmit();
				}}
				id="connection-credentials"
				name="Connection Credentials"
				class="space-y-3"
			>
				<div class="space-y-3">
					<input
						type="text"
						placeholder="Username"
						bind:value={username}
						autocomplete="username"
						class="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-800 placeholder:text-slate-400 focus:border-cyan-300 focus:outline-none"
					/>

					<div class="relative">
						{#if showPassword}
							<input
								type="text"
								placeholder="Password"
								bind:value={password}
								autocomplete="current-password"
								class="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 pr-12 text-sm text-slate-800 placeholder:text-slate-400 focus:border-cyan-300 focus:outline-none"
							/>
						{:else}
							<input
								type="password"
								placeholder="Password"
								bind:value={password}
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
							{#if showPassword}<EyeOff size={16} />{:else}<Eye size={16} />{/if}
						</button>
					</div>

					<!-- Status feedback lives here instead of a separate toast -->
					{#if isError && lastError}
						<div
							class="flex items-start gap-2.5 rounded-2xl border border-rose-200 bg-rose-50 px-4 py-3"
						>
							<WifiOff size={15} class="mt-0.5 shrink-0 text-rose-500" />
							<p class="text-xs leading-5 text-rose-700">{lastError}</p>
						</div>
					{/if}

					<button
						disabled={isConnecting || (!username && !password)}
						class="flex min-h-11 w-full items-center justify-center gap-2 rounded-full border px-4 py-2 text-sm font-semibold tracking-[0.14em] uppercase shadow-sm transition active:scale-[0.98] disabled:pointer-events-none disabled:cursor-not-allowed
						{isConnecting
							? 'border-amber-300 bg-amber-50 text-amber-700'
							: 'border-cyan-300 bg-cyan-50 text-cyan-700 hover:bg-cyan-100 disabled:border-slate-200 disabled:bg-slate-100 disabled:text-slate-400'}"
					>
						{#if isConnecting}
							<LoaderCircle size={16} class="animate-spin" />
							Connecting...
						{:else}
							<Wifi size={16} />
							Connect
						{/if}
					</button>
				</div>
			</form>
		{/if}
	</div>

	<p class="mt-8 text-center text-xs leading-5 text-slate-400">
		Credentials are stored locally on this device<br />and never sent anywhere except your broker.
	</p>
</div>
