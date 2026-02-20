<script lang="ts">
	import { onMount } from 'svelte';

	type Motor = 'borewell' | 'sump';
	type OverheadLevel = 'critical' | 'low' | 'medium' | 'high';
	type SumpLevel = 'critical' | 'low' | 'high';

	const overheadLevels: OverheadLevel[] = ['critical', 'low', 'medium', 'high'];
	const sumpLevels: SumpLevel[] = ['critical', 'low', 'high'];

	let overheadIndex = 2;
	let sumpIndex = 2;
	let manualMode = false;
	let preferredMotor: 'auto' | Motor = 'auto';
	let manualMotor: Motor = 'borewell';
	let overrideFill = false;

	let borewellDryRun = false;
	let sumpDryRun = false;
	let activeMotor: Motor | null = null;
	let status = 'System initialized';
	let cycleCount = 0;
	let alerts: string[] = [];
	let darkMode = false;

	const levelScore = {
		overhead: [15, 40, 70, 100],
		sump: [15, 50, 100]
	};

	function applyTheme(mode: boolean) {
		darkMode = mode;
		document.documentElement.classList.toggle('dark', darkMode);
	}

	onMount(() => {
		const media = window.matchMedia('(prefers-color-scheme: dark)');
		applyTheme(media.matches);
		const onSystemChange = (event: MediaQueryListEvent) => {
			applyTheme(event.matches);
		};
		media.addEventListener('change', onSystemChange);
		return () => media.removeEventListener('change', onSystemChange);
	});

	function toggleTheme() {
		applyTheme(!darkMode);
	}

	function appendAlert(message: string) {
		alerts = [message, ...alerts].slice(0, 8);
	}

	function stopMotor(reason: string) {
		if (activeMotor) {
			status = `${activeMotor} stopped: ${reason}`;
		}
		activeMotor = null;
	}

	function motorBlocked(motor: Motor) {
		if ((motor === 'borewell' && borewellDryRun) || (motor === 'sump' && sumpDryRun)) {
			return 'dry-run protection';
		}
		if (!manualMode && motor === 'sump' && sumpIndex === 0) {
			return 'sump below critical';
		}
		return null;
	}

	function motorPriority(): Motor[] {
		if (manualMode) {
			return [manualMotor, manualMotor === 'borewell' ? 'sump' : 'borewell'];
		}
		if (preferredMotor === 'borewell') return ['borewell', 'sump'];
		if (preferredMotor === 'sump') return ['sump', 'borewell'];
		return ['borewell', 'sump'];
	}

	function evaluateAutomation() {
		cycleCount += 1;
		const overheadNeedsFill = overheadIndex <= 1;
		const shouldFill = overrideFill || overheadNeedsFill;

		if (sumpIndex === 0) appendAlert('Sump level is CRITICAL');

		if (!shouldFill && !manualMode) {
			stopMotor('overhead tank is healthy');
			return;
		}

		if (manualMode && !activeMotor) {
			const blocked = motorBlocked(manualMotor);
			if (blocked) {
				status = `${manualMotor} cannot start: ${blocked}`;
				return;
			}
			activeMotor = manualMotor;
			status = `manual mode: ${activeMotor} started`;
			return;
		}

		for (const motor of motorPriority()) {
			const blocked = motorBlocked(motor);
			if (blocked) {
				appendAlert(`${motor} unavailable (${blocked})`);
				continue;
			}
			activeMotor = motor;
			status = `${motor} running (${overrideFill ? 'override' : 'automatic'})`;
			return;
		}

		activeMotor = null;
		status = 'No motor can run with current safety constraints';
	}

	function simulateMinute() {
		if (!activeMotor) {
			status = 'No active motor; run automation to select one';
			return;
		}

		overheadIndex = Math.min(overheadIndex + 1, overheadLevels.length - 1);
		if (activeMotor === 'sump') sumpIndex = Math.max(sumpIndex - 1, 0);

		if (overheadIndex === 3) {
			overrideFill = false;
			stopMotor('overhead tank reached HIGH');
			return;
		}

		if (!manualMode && activeMotor === 'sump' && sumpIndex === 0) {
			stopMotor('sump dropped below critical guardrail');
			return;
		}

		status = `${activeMotor} still running; fill cycle in progress`;
	}

	function forceFill() {
		overrideFill = true;
		evaluateAutomation();
	}
</script>

<svelte:head>
	<title>Water Level Automation Console</title>
</svelte:head>

<div
	class="min-h-screen bg-slate-100 text-slate-900 transition-colors dark:bg-slate-950 dark:text-slate-100"
>
	<div class="mx-auto max-w-7xl px-4 py-6 md:px-8">
		<header
			class="mb-6 flex flex-col gap-4 rounded-2xl border border-slate-200 bg-white px-5 py-4 shadow-sm md:flex-row md:items-center md:justify-between dark:border-slate-800 dark:bg-slate-900"
		>
			<div>
				<h1 class="text-2xl font-bold tracking-tight">Water Level Automation Console</h1>
				<p class="text-sm text-slate-600 dark:text-slate-300">
					Arduino UNO R4 WiFi + SvelteKit PWA
				</p>
			</div>
			<button
				on:click={toggleTheme}
				class="inline-flex items-center gap-2 rounded-xl border border-slate-300 bg-slate-100 px-4 py-2 text-sm font-semibold text-slate-800 transition hover:bg-slate-200 dark:border-slate-700 dark:bg-slate-800 dark:text-slate-100 dark:hover:bg-slate-700"
			>
				{#if darkMode}
					<svg
						class="h-4 w-4"
						viewBox="0 0 24 24"
						fill="none"
						stroke="currentColor"
						stroke-width="2"
					>
						<path d="M12 3a1 1 0 0 1 1 1v1a1 1 0 1 1-2 0V4a1 1 0 0 1 1-1Z" />
						<path d="M12 18a6 6 0 1 0 0-12 6 6 0 0 0 0 12Z" />
						<path
							d="M5.64 5.64a1 1 0 0 1 1.41 0l.71.7a1 1 0 1 1-1.41 1.42l-.7-.71a1 1 0 0 1 0-1.41Z"
						/>
						<path d="M3 12a1 1 0 0 1 1-1h1a1 1 0 1 1 0 2H4a1 1 0 0 1-1-1Z" />
						<path d="M18.36 5.64a1 1 0 0 1 1.41 1.41l-.7.71a1 1 0 1 1-1.42-1.41l.71-.71Z" />
						<path d="M19 12a1 1 0 0 1 1-1h1a1 1 0 1 1 0 2h-1a1 1 0 0 1-1-1Z" />
					</svg>
					Light mode
				{:else}
					<svg
						class="h-4 w-4"
						viewBox="0 0 24 24"
						fill="none"
						stroke="currentColor"
						stroke-width="2"
					>
						<path d="M12 3a9 9 0 1 0 9 9 7 7 0 0 1-9-9Z" />
					</svg>
					Dark mode
				{/if}
			</button>
		</header>

		<section class="grid gap-4 lg:grid-cols-3">
			<article
				class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm dark:border-slate-800 dark:bg-slate-900"
			>
				<div class="mb-4 flex items-center gap-2">
					<svg
						class="h-5 w-5 text-blue-600"
						viewBox="0 0 24 24"
						fill="none"
						stroke="currentColor"
						stroke-width="2"
					>
						<path d="M12 2v20" />
						<path d="M5 8h14" />
						<path d="M7 14h10" />
					</svg>
					<h2 class="font-semibold">Tank Levels</h2>
				</div>

				<label for="overhead-tank" class="mb-3 block text-sm font-medium">Overhead Tank</label>
				<select
					id="overhead-tank"
					bind:value={overheadIndex}
					class="w-full rounded-xl border-slate-300 bg-slate-50 text-sm dark:border-slate-700 dark:bg-slate-800"
				>
					{#each overheadLevels as level, index (level)}
						<option value={index}>{level}</option>
					{/each}
				</select>
				<div class="mt-2 h-2 rounded-full bg-slate-200 dark:bg-slate-700">
					<div
						class="h-2 rounded-full bg-blue-600 transition-all"
						style={`width: ${levelScore.overhead[overheadIndex]}%`}
					></div>
				</div>

				<label for="sump-tank" class="mt-4 mb-3 block text-sm font-medium">Sump Tank</label>
				<select
					id="sump-tank"
					bind:value={sumpIndex}
					class="w-full rounded-xl border-slate-300 bg-slate-50 text-sm dark:border-slate-700 dark:bg-slate-800"
				>
					{#each sumpLevels as level, index (level)}
						<option value={index}>{level}</option>
					{/each}
				</select>
				<div class="mt-2 h-2 rounded-full bg-slate-200 dark:bg-slate-700">
					<div
						class="h-2 rounded-full bg-emerald-600 transition-all"
						style={`width: ${levelScore.sump[sumpIndex]}%`}
					></div>
				</div>
			</article>

			<article
				class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm dark:border-slate-800 dark:bg-slate-900"
			>
				<div class="mb-4 flex items-center gap-2">
					<svg
						class="h-5 w-5 text-violet-600"
						viewBox="0 0 24 24"
						fill="none"
						stroke="currentColor"
						stroke-width="2"
					>
						<path d="M12 20V10" />
						<path d="m18 14-6-6-6 6" />
					</svg>
					<h2 class="font-semibold">Commands</h2>
				</div>
				<label
					class="mb-3 flex items-center gap-2 rounded-xl bg-slate-100 p-3 text-sm dark:bg-slate-800"
				>
					<input type="checkbox" bind:checked={manualMode} class="rounded" />
					<span>Manual mode (tank-level safety bypassed)</span>
				</label>

				<label for="preferred-motor" class="mb-1 block text-sm font-medium">Preferred Motor</label>
				<select
					id="preferred-motor"
					bind:value={preferredMotor}
					disabled={manualMode}
					class="w-full rounded-xl border-slate-300 bg-slate-50 text-sm disabled:opacity-50 dark:border-slate-700 dark:bg-slate-800"
				>
					<option value="auto">auto (borewell priority)</option>
					<option value="borewell">force borewell first</option>
					<option value="sump">force sump first</option>
				</select>

				<label for="manual-motor" class="mt-4 mb-1 block text-sm font-medium">Manual Motor</label>
				<select
					id="manual-motor"
					bind:value={manualMotor}
					disabled={!manualMode}
					class="w-full rounded-xl border-slate-300 bg-slate-50 text-sm disabled:opacity-50 dark:border-slate-700 dark:bg-slate-800"
				>
					<option value="borewell">borewell</option>
					<option value="sump">sump</option>
				</select>

				<div class="mt-4 grid gap-2">
					<button
						on:click={evaluateAutomation}
						class="rounded-xl bg-blue-600 px-4 py-2 text-sm font-semibold text-white transition hover:bg-blue-700"
					>
						Run Automation Cycle
					</button>
					<button
						on:click={forceFill}
						class="rounded-xl bg-indigo-600 px-4 py-2 text-sm font-semibold text-white transition hover:bg-indigo-700"
					>
						Force Start / Override
					</button>
					<button
						on:click={simulateMinute}
						class="rounded-xl border border-slate-300 bg-white px-4 py-2 text-sm font-semibold transition hover:bg-slate-100 dark:border-slate-700 dark:bg-slate-800 dark:hover:bg-slate-700"
					>
						Simulate 1 Minute
					</button>
				</div>
			</article>

			<article
				class="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm dark:border-slate-800 dark:bg-slate-900"
			>
				<div class="mb-4 flex items-center gap-2">
					<svg
						class="h-5 w-5 text-amber-600"
						viewBox="0 0 24 24"
						fill="none"
						stroke="currentColor"
						stroke-width="2"
					>
						<path d="M12 9v4" />
						<path d="M12 17h.01" />
						<path
							d="M10.29 3.86 1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3l-8.47-14.14a2 2 0 0 0-3.42 0z"
						/>
					</svg>
					<h2 class="font-semibold">Safety Guard Rails</h2>
				</div>

				<label
					class="mb-3 flex items-center gap-2 rounded-xl bg-slate-100 p-3 text-sm dark:bg-slate-800"
				>
					<input type="checkbox" bind:checked={borewellDryRun} class="rounded" />
					<span>Borewell dry-run detected</span>
				</label>
				<label
					class="flex items-center gap-2 rounded-xl bg-slate-100 p-3 text-sm dark:bg-slate-800"
				>
					<input type="checkbox" bind:checked={sumpDryRun} class="rounded" />
					<span>Sump motor dry-run detected</span>
				</label>
				<p
					class="mt-4 rounded-xl border border-amber-300 bg-amber-50 p-3 text-xs text-amber-800 dark:border-amber-700 dark:bg-amber-950/40 dark:text-amber-200"
				>
					If sump falls below critical, automatic mode will stop sump pumping.
				</p>
			</article>
		</section>

		<section
			class="mt-4 rounded-2xl border border-slate-200 bg-white p-5 shadow-sm dark:border-slate-800 dark:bg-slate-900"
		>
			<div class="mb-3 flex items-center gap-2">
				<svg
					class="h-5 w-5 text-emerald-600"
					viewBox="0 0 24 24"
					fill="none"
					stroke="currentColor"
					stroke-width="2"
				>
					<path d="M3 3v18h18" />
					<path d="m19 9-5 5-4-4-3 3" />
				</svg>
				<h2 class="font-semibold">Live Status</h2>
			</div>

			<div class="grid gap-3 md:grid-cols-2 xl:grid-cols-5">
				<div class="rounded-xl bg-slate-100 p-3 dark:bg-slate-800">
					<span class="text-xs">Cycle</span>
					<p class="font-semibold">{cycleCount}</p>
				</div>
				<div class="rounded-xl bg-slate-100 p-3 dark:bg-slate-800">
					<span class="text-xs">Overhead</span>
					<p class="font-semibold capitalize">{overheadLevels[overheadIndex]}</p>
				</div>
				<div class="rounded-xl bg-slate-100 p-3 dark:bg-slate-800">
					<span class="text-xs">Sump</span>
					<p class="font-semibold capitalize">{sumpLevels[sumpIndex]}</p>
				</div>
				<div class="rounded-xl bg-slate-100 p-3 dark:bg-slate-800">
					<span class="text-xs">Motor</span>
					<p class="font-semibold capitalize">{activeMotor ?? 'stopped'}</p>
				</div>
				<div class="rounded-xl bg-slate-100 p-3 dark:bg-slate-800">
					<span class="text-xs">Mode</span>
					<p class="font-semibold">{manualMode ? 'manual' : 'auto'}</p>
				</div>
			</div>

			<p
				class="mt-4 rounded-xl border border-blue-300 bg-blue-50 p-3 text-sm text-blue-900 dark:border-blue-700 dark:bg-blue-950/40 dark:text-blue-100"
			>
				{status}
			</p>

			<h3 class="mt-4 text-sm font-semibold">Latest Alerts</h3>
			<ul class="mt-2 space-y-2 text-sm">
				{#if alerts.length === 0}
					<li
						class="rounded-lg bg-slate-100 p-2 text-slate-600 dark:bg-slate-800 dark:text-slate-300"
					>
						No alerts
					</li>
				{:else}
					{#each alerts as alert, i (`${alert}-${i}`)}
						<li
							class="rounded-lg bg-rose-50 p-2 text-rose-700 dark:bg-rose-950/40 dark:text-rose-200"
						>
							{alert}
						</li>
					{/each}
				{/if}
			</ul>
		</section>
	</div>
</div>
