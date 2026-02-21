<script lang="ts">
	import { waterSystem } from '../lib/stores/system.js';
	import type { Mode, Motor, OverheadLevel, SumpLevel } from '../lib/types.js';

	const modeOptions: Mode[] = ['AUTO', 'OVERRIDE_FILL', 'MANUAL'];
	const overheadOptions: OverheadLevel[] = ['CRITICAL', 'LOW', 'MEDIUM', 'HIGH'];
	const sumpOptions: SumpLevel[] = ['BELOW_CRITICAL', 'CRITICAL', 'LOW', 'HIGH'];
	const motors: Motor[] = ['BOREWELL', 'SUMP'];

	const modeLabel: Record<Mode, string> = {
		AUTO: 'Automatic',
		OVERRIDE_FILL: 'Override Fill',
		MANUAL: 'Manual'
	};
</script>

<svelte:head>
	<title>Water Automation PWA</title>
</svelte:head>

<div class="min-h-screen bg-slate-100 px-4 py-6 text-slate-900 md:px-8">
	<div class="mx-auto grid max-w-7xl gap-4">
		<header class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
			<h1 class="text-2xl font-bold">Water Level Automation PWA</h1>
			<p class="mt-1 text-sm text-slate-600">
				Phase 1 UI: mode control, safety inputs, motor priority flow, and live telemetry.
			</p>
		</header>

		<section class="grid gap-4 lg:grid-cols-3">
			<article class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm lg:col-span-2">
				<h2 class="text-base font-semibold">System Telemetry</h2>
				<div class="mt-3 grid gap-3 sm:grid-cols-2 xl:grid-cols-4">
					<div class="rounded-xl bg-slate-100 p-3">
						<p class="text-xs text-slate-500">Mode</p>
						<p class="font-semibold">{modeLabel[$waterSystem.mode]}</p>
					</div>
					<div class="rounded-xl bg-slate-100 p-3">
						<p class="text-xs text-slate-500">Active Motor</p>
						<p class="font-semibold">{$waterSystem.activeMotor ?? 'NONE'}</p>
					</div>
					<div class="rounded-xl bg-slate-100 p-3">
						<p class="text-xs text-slate-500">Overhead Tank</p>
						<p class="font-semibold">{$waterSystem.overhead}</p>
					</div>
					<div class="rounded-xl bg-slate-100 p-3">
						<p class="text-xs text-slate-500">Sump Tank</p>
						<p class="font-semibold">{$waterSystem.sump}</p>
					</div>
				</div>

				<div class="mt-3 rounded-xl bg-slate-100 p-3 text-sm">
					<p><span class="font-semibold">Cycle count:</span> {$waterSystem.cycleCount}</p>
					<p>
						<span class="font-semibold">Need fill:</span>
						{$waterSystem.needFill ? 'Yes' : 'No'}
					</p>
					<p><span class="font-semibold">Last event:</span> {$waterSystem.lastEvent}</p>
				</div>
			</article>

			<article class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
				<h2 class="text-base font-semibold">Safety Alarms</h2>
				<ul class="mt-3 space-y-2 text-sm">
					<li
						class="rounded-lg p-2"
						class:bg-rose-100={$waterSystem.alarms.overheadCritical}
						class:bg-emerald-100={!$waterSystem.alarms.overheadCritical}
					>
						Overhead critical: {$waterSystem.alarms.overheadCritical ? 'YES' : 'NO'}
					</li>
					<li
						class="rounded-lg p-2"
						class:bg-rose-100={$waterSystem.alarms.sumpCritical}
						class:bg-emerald-100={!$waterSystem.alarms.sumpCritical}
					>
						Sump critical: {$waterSystem.alarms.sumpCritical ? 'YES' : 'NO'}
					</li>
					<li
						class="rounded-lg p-2"
						class:bg-rose-100={$waterSystem.alarms.sumpBelowCritical}
						class:bg-emerald-100={!$waterSystem.alarms.sumpBelowCritical}
					>
						Sump below critical: {$waterSystem.alarms.sumpBelowCritical ? 'YES' : 'NO'}
					</li>
				</ul>
			</article>
		</section>

		<section class="grid gap-4 lg:grid-cols-3">
			<article class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
				<h3 class="text-sm font-semibold">Mode and Tank Inputs</h3>
				<div class="mt-3 space-y-3 text-sm">
					<label class="block">
						<span class="mb-1 block">Mode</span>
						<select
							class="w-full rounded-lg border-slate-300 bg-slate-50"
							on:change={(e) =>
								waterSystem.setMode((e.currentTarget as HTMLSelectElement).value as Mode)}
							value={$waterSystem.mode}
						>
							{#each modeOptions as mode (mode)}
								<option value={mode}>{modeLabel[mode]}</option>
							{/each}
						</select>
					</label>

					<label class="block">
						<span class="mb-1 block">Overhead level</span>
						<select
							class="w-full rounded-lg border-slate-300 bg-slate-50"
							on:change={(e) =>
								waterSystem.setOverheadLevel(
									(e.currentTarget as HTMLSelectElement).value as OverheadLevel
								)}
							value={$waterSystem.overhead}
						>
							{#each overheadOptions as level (level)}
								<option value={level}>{level}</option>
							{/each}
						</select>
					</label>

					<label class="block">
						<span class="mb-1 block">Sump level</span>
						<select
							class="w-full rounded-lg border-slate-300 bg-slate-50"
							on:change={(e) =>
								waterSystem.setSumpLevel((e.currentTarget as HTMLSelectElement).value as SumpLevel)}
							value={$waterSystem.sump}
						>
							{#each sumpOptions as level (level)}
								<option value={level}>{level}</option>
							{/each}
						</select>
					</label>
				</div>
			</article>

			<article class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
				<h3 class="text-sm font-semibold">Motor Safety Sensors</h3>
				<div class="mt-3 space-y-3 text-sm">
					{#each motors as motor (motor)}
						<label class="flex items-center justify-between rounded-lg bg-slate-100 p-3">
							<span>{motor} dry-run</span>
							<input
								type="checkbox"
								checked={$waterSystem.motors[motor].dryRunDetected}
								on:change={(e) =>
									waterSystem.setDryRun(motor, (e.currentTarget as HTMLInputElement).checked)}
							/>
						</label>
					{/each}
				</div>
			</article>

			<article class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
				<h3 class="text-sm font-semibold">Actions</h3>
				<div class="mt-3 grid gap-2">
					<button
						class="rounded-lg bg-blue-600 px-3 py-2 text-sm font-medium text-white"
						on:click={waterSystem.runAutomation}
					>
						Run automation
					</button>
					<button
						class="rounded-lg bg-indigo-600 px-3 py-2 text-sm font-medium text-white"
						on:click={waterSystem.forceFill}
					>
						Force fill (override)
					</button>
					<button
						class="rounded-lg bg-emerald-600 px-3 py-2 text-sm font-medium text-white"
						on:click={waterSystem.simulateMinute}
					>
						Simulate +1 minute
					</button>
					<button
						class="rounded-lg bg-amber-600 px-3 py-2 text-sm font-medium text-white"
						on:click={() => waterSystem.manualStart('BOREWELL')}
					>
						Manual start borewell
					</button>
					<button
						class="rounded-lg bg-amber-700 px-3 py-2 text-sm font-medium text-white"
						on:click={() => waterSystem.manualStart('SUMP')}
					>
						Manual start sump
					</button>
					<button
						class="rounded-lg bg-rose-700 px-3 py-2 text-sm font-medium text-white"
						on:click={waterSystem.stopAll}
					>
						Emergency stop
					</button>
				</div>
			</article>
		</section>
	</div>
</div>
