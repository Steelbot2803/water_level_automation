<script lang="ts">
	import { onMount } from 'svelte';

	import {
		commandLabels,
		controlModeLabels,
		motorLabels,
		runtimeStatusLabels
	} from '../lib/control.js';
	import { waterSystem } from '../lib/stores/system.js';
	import type { ArduinoCommand, ConnectionPhase } from '../lib/types.js';

	const commandButtons: Array<{
		command: ArduinoCommand;
		description: string;
		className: string;
	}> = [
		{
			command: 'auto',
			description: 'Return the controller to automatic mode.',
			className: 'bg-blue-700 text-white'
		},
		{
			command: 'override',
			description: 'Fill to HIGH regardless of current overhead level.',
			className: 'bg-cyan-700 text-white'
		},
		{
			command: 'manual',
			description: 'Enter manual mode without starting a motor.',
			className: 'bg-amber-600 text-white'
		},
		{
			command: 'motor borewell',
			description: 'Manual command: run the borewell motor.',
			className: 'bg-emerald-700 text-white'
		},
		{
			command: 'motor sump',
			description: 'Manual command: run the sump transfer motor.',
			className: 'bg-lime-700 text-white'
		},
		{
			command: 'motor stop',
			description: 'Stop any commanded motor output.',
			className: 'bg-rose-700 text-white'
		},
		{
			command: 'status',
			description: 'Trigger the Arduino serial status printout.',
			className: 'bg-slate-700 text-white'
		}
	];

	const connectionBadges: Record<ConnectionPhase, string> = {
		idle: 'bg-slate-200 text-slate-700',
		connecting: 'bg-amber-100 text-amber-900',
		connected: 'bg-emerald-100 text-emerald-900',
		reconnecting: 'bg-orange-100 text-orange-900',
		offline: 'bg-rose-100 text-rose-900',
		error: 'bg-rose-200 text-rose-950'
	};

	onMount(() => {
		waterSystem.initialize();
	});

	function formatTimestamp(value?: number) {
		if (!value) return 'Not available';

		return new Intl.DateTimeFormat(undefined, {
			dateStyle: 'medium',
			timeStyle: 'medium'
		}).format(value);
	}
</script>

<svelte:head>
	<title>Water Level Automation</title>
</svelte:head>

<div class="min-h-screen bg-stone-100 px-4 py-6 text-slate-950 md:px-8">
	<div class="mx-auto grid max-w-7xl gap-4">
		<header class="rounded-3xl border border-stone-200 bg-white p-6 shadow-sm">
			<div class="flex flex-col gap-4 lg:flex-row lg:items-start lg:justify-between">
				<div>
					<p class="text-xs font-semibold uppercase tracking-[0.2em] text-cyan-800">
						Water Level Automation
					</p>
					<h1 class="mt-2 text-3xl font-semibold tracking-tight">HiveMQ Control Surface</h1>
					<p class="mt-2 max-w-3xl text-sm text-slate-600">
						This PWA now auto-connects from `pwa/.env` using the browser WebSocket endpoint.
						Command publishing still matches the Arduino command parser exactly.
					</p>
				</div>

				<div class="rounded-2xl bg-stone-100 p-4 text-sm">
					<div class="flex items-center gap-2">
						<span
							class={`rounded-full px-3 py-1 text-xs font-semibold uppercase tracking-wide ${connectionBadges[$waterSystem.connection.phase]}`}
						>
							{$waterSystem.connection.phase}
						</span>
					</div>
					<p class="mt-3 max-w-md text-slate-700">{$waterSystem.connection.detail}</p>
					<p class="mt-2 text-xs text-slate-500">
						Last broker connect: {formatTimestamp($waterSystem.connection.lastConnectedAt)}
					</p>
					<p class="mt-1 text-xs text-slate-500">
						Last device update: {formatTimestamp($waterSystem.connection.lastMessageAt)}
					</p>
				</div>
			</div>
		</header>

		<section class="grid gap-4 xl:grid-cols-[0.9fr,1.5fr]">
			<article class="rounded-3xl border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex items-center justify-between">
					<div>
						<h2 class="text-base font-semibold">Broker Configuration</h2>
						<p class="mt-1 text-sm text-slate-600">
							Read from `pwa/.env` at startup. Edit the file and restart the dev server if you
							change these values.
						</p>
					</div>
					<div class="flex gap-2">
						<button
							class="rounded-xl bg-cyan-800 px-4 py-2 text-sm font-medium text-white"
							on:click={waterSystem.connect}
						>
							Reconnect
						</button>
						<button
							class="rounded-xl border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700"
							on:click={waterSystem.disconnect}
						>
							Disconnect
						</button>
					</div>
				</div>

				<div class="mt-4 grid gap-3 text-sm sm:grid-cols-2">
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">Host</p>
						<p class="mt-1 break-all font-semibold">{$waterSystem.settings.host || 'Not set'}</p>
					</div>
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">Port</p>
						<p class="mt-1 font-semibold">{$waterSystem.settings.port}</p>
					</div>
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">Path</p>
						<p class="mt-1 font-semibold">{$waterSystem.settings.path}</p>
					</div>
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">TLS / WSS</p>
						<p class="mt-1 font-semibold">{$waterSystem.settings.useSSL ? 'Enabled' : 'Disabled'}</p>
					</div>
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">Command Topic</p>
						<p class="mt-1 break-all font-semibold">{$waterSystem.settings.commandTopic}</p>
					</div>
					<div class="rounded-2xl bg-stone-100 p-4">
						<p class="text-xs uppercase tracking-wide text-slate-500">Status Topic</p>
						<p class="mt-1 break-all font-semibold">{$waterSystem.settings.statusTopic}</p>
					</div>
				</div>

				<div class="mt-4 rounded-2xl bg-stone-100 p-4 text-sm text-slate-700">
					<p><span class="font-semibold">Broker URL:</span> {$waterSystem.connection.url || 'Not configured'}</p>
					<p class="mt-1">
						<span class="font-semibold">Status subscription:</span>
						{$waterSystem.statusTopicSubscribed ? 'Active' : 'Inactive'}
					</p>
					{#if $waterSystem.connection.lastError}
						<p class="mt-2 text-rose-700">
							<span class="font-semibold">Last error:</span> {$waterSystem.connection.lastError}
						</p>
					{/if}
				</div>
			</article>

			<article class="rounded-3xl border border-stone-200 bg-white p-5 shadow-sm">
				<div class="flex items-center justify-between">
					<div>
						<h2 class="text-base font-semibold">Live Telemetry</h2>
						<p class="mt-1 text-sm text-slate-600">
							Parsed from the Arduino MQTT status payload and aligned with the controller logic.
						</p>
					</div>
					{#if $waterSystem.device}
						<p class="text-xs text-slate-500">
							Updated {formatTimestamp($waterSystem.device.receivedAt)}
						</p>
					{/if}
				</div>

				{#if $waterSystem.device}
					<div class="mt-4 grid gap-3 sm:grid-cols-2 xl:grid-cols-3">
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Control Mode</p>
							<p class="mt-1 text-lg font-semibold">
								{controlModeLabels[$waterSystem.device.controlMode]}
							</p>
						</div>
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Active Motor</p>
							<p class="mt-1 text-lg font-semibold">{motorLabels[$waterSystem.device.motor]}</p>
						</div>
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Manual Target</p>
							<p class="mt-1 text-lg font-semibold">
								{motorLabels[$waterSystem.device.manual_target ?? 'none']}
							</p>
						</div>
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Overhead Tank</p>
							<p class="mt-1 text-lg font-semibold">{$waterSystem.device.overhead}</p>
						</div>
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Sump Tank</p>
							<p class="mt-1 text-lg font-semibold">{$waterSystem.device.sump}</p>
						</div>
						<div class="rounded-2xl bg-stone-100 p-4">
							<p class="text-xs uppercase tracking-wide text-slate-500">Need Fill</p>
							<p class="mt-1 text-lg font-semibold">
								{$waterSystem.device.needFill ? 'Yes' : 'No'}
							</p>
						</div>
					</div>

					<div class="mt-4 grid gap-4 lg:grid-cols-2">
						<div class="rounded-2xl bg-stone-100 p-4">
							<h3 class="text-sm font-semibold">Motor Runtime State</h3>
							<div class="mt-3 grid gap-3 sm:grid-cols-2">
								<div class="rounded-2xl bg-white p-4">
									<p class="text-xs uppercase tracking-wide text-slate-500">Borewell</p>
									<p class="mt-1 font-semibold">
										{runtimeStatusLabels[$waterSystem.device.motors.borewell.status]}
									</p>
									<p class="mt-1 text-xs text-slate-500">
										{$waterSystem.device.motors.borewell.active ? 'Active output' : 'Inactive'}
									</p>
								</div>
								<div class="rounded-2xl bg-white p-4">
									<p class="text-xs uppercase tracking-wide text-slate-500">Sump Transfer</p>
									<p class="mt-1 font-semibold">
										{runtimeStatusLabels[$waterSystem.device.motors.sumpTransfer.status]}
									</p>
									<p class="mt-1 text-xs text-slate-500">
										{$waterSystem.device.motors.sumpTransfer.active ? 'Active output' : 'Inactive'}
									</p>
								</div>
							</div>
						</div>

						<div class="rounded-2xl bg-stone-100 p-4">
							<h3 class="text-sm font-semibold">Safety Flags</h3>
							<ul class="mt-3 grid gap-2 text-sm">
								<li
									class={`rounded-xl px-3 py-2 ${$waterSystem.device.alarms.overheadCritical ? 'bg-rose-100 text-rose-900' : 'bg-emerald-100 text-emerald-900'}`}
								>
									Overhead critical: {$waterSystem.device.alarms.overheadCritical ? 'YES' : 'NO'}
								</li>
								<li
									class={`rounded-xl px-3 py-2 ${$waterSystem.device.alarms.sumpCritical ? 'bg-rose-100 text-rose-900' : 'bg-emerald-100 text-emerald-900'}`}
								>
									Sump critical: {$waterSystem.device.alarms.sumpCritical ? 'YES' : 'NO'}
								</li>
								<li
									class={`rounded-xl px-3 py-2 ${$waterSystem.device.alarms.sumpBelowCritical ? 'bg-rose-100 text-rose-900' : 'bg-emerald-100 text-emerald-900'}`}
								>
									Sump below critical: {$waterSystem.device.alarms.sumpBelowCritical ? 'YES' : 'NO'}
								</li>
								<li
									class={`rounded-xl px-3 py-2 ${$waterSystem.device.alarms.sumpWarning ? 'bg-amber-100 text-amber-900' : 'bg-emerald-100 text-emerald-900'}`}
								>
									Sump warning latch: {$waterSystem.device.alarms.sumpWarning ? 'ACTIVE' : 'CLEAR'}
								</li>
							</ul>
						</div>
					</div>

					<div class="mt-4 rounded-2xl bg-slate-950 p-4 text-sm text-slate-100">
						<p class="text-xs uppercase tracking-wide text-slate-400">Raw MQTT Payload</p>
						<pre class="mt-3 overflow-x-auto whitespace-pre-wrap font-mono text-xs">{$waterSystem.device.rawPayload}</pre>
					</div>
				{:else}
					<div class="mt-4 rounded-2xl bg-stone-100 p-5 text-sm text-slate-600">
						<p>No device telemetry has arrived yet.</p>
						<p class="mt-2">
							If the broker connection is healthy, the Arduino should publish a retained status message to
							the configured status topic every 2 seconds.
						</p>
					</div>
				{/if}
			</article>
		</section>

		<section class="grid gap-4 xl:grid-cols-[1.2fr,0.8fr]">
			<article class="rounded-3xl border border-stone-200 bg-white p-5 shadow-sm">
				<h2 class="text-base font-semibold">Commands</h2>
				<p class="mt-1 text-sm text-slate-600">
					These buttons publish the exact command strings accepted by `applyCommand()` in the Arduino
					sketch.
				</p>

				<div class="mt-4 grid gap-3 md:grid-cols-2 xl:grid-cols-3">
					{#each commandButtons as button (button.command)}
						<button
							class={`rounded-2xl px-4 py-4 text-left shadow-sm transition hover:shadow-md disabled:cursor-not-allowed disabled:bg-slate-300 disabled:text-slate-600 ${button.className}`}
							disabled={$waterSystem.connection.phase !== 'connected'}
							on:click={() => waterSystem.sendCommand(button.command)}
						>
							<p class="text-sm font-semibold">{commandLabels[button.command]}</p>
							<p class="mt-2 text-xs/5 opacity-90">{button.description}</p>
						</button>
					{/each}
				</div>
			</article>

			<article class="rounded-3xl border border-stone-200 bg-white p-5 shadow-sm">
				<h2 class="text-base font-semibold">Command History</h2>
				<p class="mt-1 text-sm text-slate-600">Recent publishes from this browser session.</p>

				{#if $waterSystem.recentCommands.length}
					<ul class="mt-4 space-y-2 text-sm">
						{#each $waterSystem.recentCommands as entry (entry.id)}
							<li class="rounded-2xl bg-stone-100 p-3">
								<div class="flex items-center justify-between gap-3">
									<p class="font-semibold">{entry.command}</p>
									<span
										class={`rounded-full px-2 py-1 text-xs font-semibold uppercase tracking-wide ${entry.status === 'sent' ? 'bg-emerald-100 text-emerald-900' : 'bg-rose-100 text-rose-900'}`}
									>
										{entry.status}
									</span>
								</div>
								<p class="mt-1 text-xs text-slate-500">{formatTimestamp(entry.at)}</p>
								{#if entry.error}
									<p class="mt-2 text-xs text-rose-700">{entry.error}</p>
								{/if}
							</li>
						{/each}
					</ul>
				{:else}
					<div class="mt-4 rounded-2xl bg-stone-100 p-4 text-sm text-slate-600">
						No commands sent from this client yet.
					</div>
				{/if}
			</article>
		</section>
	</div>
</div>
