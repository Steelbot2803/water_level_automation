<svelte:options runes={true} />

<script lang="ts">
	import { Aperture, LoaderCircle, Lock, TriangleAlert } from 'lucide-svelte';
	import type { MotorRuntimeStatus } from '$lib/types.js';
	import { runtimeStatusLabels } from '$lib/control.js';

	let {
		label,
		status,
		onUnlock = undefined,
		unlockDisabled = false
	}: {
		label: string;
		status: MotorRuntimeStatus | undefined;
		onUnlock?: (() => void) | undefined;
		unlockDisabled?: boolean;
	} = $props();

	const resolvedStatus = $derived(status ?? 'stopped');

	const isStarting = $derived(resolvedStatus === 'starting');
	const isRunning = $derived(resolvedStatus === 'running');
	const isSpinningDown = $derived(resolvedStatus === 'sump_critical');
	const isLocked = $derived(
		resolvedStatus === 'dry_run_lock' || resolvedStatus === 'sump_critical'
	);
	const isNull = $derived(status == null || status === undefined);

	const StatusIcon = $derived(
		resolvedStatus === 'dry_run_lock'
			? Lock
			: resolvedStatus === 'sump_critical'
				? TriangleAlert
				: LoaderCircle
	);

	const ringClass = $derived(
		isRunning
			? 'border-emerald-300'
			: isStarting
				? 'border-amber-300'
				: isLocked
					? 'border-rose-300'
					: 'border-slate-300'
	);

	const hubClass = $derived(
		isRunning
			? 'bg-emerald-100'
			: isStarting
				? 'bg-amber-100'
				: isLocked
					? 'bg-rose-100'
					: 'bg-white'
	);

	const bladeClass = $derived(
		isRunning
			? 'text-emerald-950/70'
			: isStarting
				? 'text-amber-950/65'
				: isLocked
					? 'text-rose-950/65'
					: 'text-slate-950/55'
	);

	const labelClass = $derived(
		isRunning
			? 'text-emerald-600'
			: isStarting
				? 'text-amber-600'
				: isLocked
					? 'text-rose-600'
					: 'text-slate-950'
	);

	const iconClass = $derived(
		StatusIcon === Lock || StatusIcon === TriangleAlert ? 'text-rose-700' : 'text-slate-300'
	);

	const discGradient = $derived(
		isRunning
			? 'linear-gradient(145deg, #059669, #34d399)'
			: isStarting
				? 'linear-gradient(145deg, #d97706, #fbbf24)'
				: isLocked
					? 'linear-gradient(145deg, #e11d48, #fb7185)'
					: 'linear-gradient(145deg, #94a3b8, #cbd5e1)'
	);

	const discSheen = $derived(
		isRunning
			? 'radial-gradient(circle at 35% 30%, rgba(167,243,208,0.35) 0%, transparent 65%)'
			: isStarting
				? 'radial-gradient(circle at 35% 30%, rgba(253,230,138,0.35) 0%, transparent 65%)'
				: isLocked
					? 'radial-gradient(circle at 35% 30%, rgba(254,205,211,0.30) 0%, transparent 65%)'
					: 'radial-gradient(circle at 35% 30%, rgba(255,255,255,0.28) 0%, transparent 65%)'
	);

	// ── WAAPI spin control ─────────────────────────────────────────────────────
	//
	// One persistent Animation object lives for the lifetime of the component.
	// We never recreate it — only its playbackRate changes, so the disc angle
	// never resets to 0° between state transitions.
	//
	// updatePlaybackRate() sounds like it interpolates but in practice it snaps.
	// We drive the ramp ourselves with requestAnimationFrame instead.

	let discEl = $state<HTMLElement | null>(null);
	let spinAnim: Animation | null = null;
	let rampRafId: number | null = null;

	function getAnim(): Animation {
		if (spinAnim) return spinAnim;
		spinAnim = discEl!.animate(
			[{ transform: 'rotate(0deg)' }, { transform: 'rotate(360deg)' }],
			// duration 250 ms at rate 1 = 1 rev/s. easing is linear so the
			// playbackRate is the sole driver of perceived speed.
			{ duration: 250, iterations: Infinity, easing: 'linear' }
		);
		spinAnim.pause();
		return spinAnim;
	}

	// Interpolates playbackRate from its current value to `target` over `durationMs`.
	// easingFn defaults to ease-out on ramp-up (rate climbs fast, settles slowly)
	// and ease-in on ramp-down (rate drops slowly at first, fast at the end).
	// Both feel physically correct — inertia resists starting and stopping.
	function rampRate(anim: Animation, target: number, durationMs: number, onDone?: () => void) {
		if (rampRafId !== null) {
			cancelAnimationFrame(rampRafId);
			rampRafId = null;
		}

		const start = anim.playbackRate;
		const delta = target - start;

		if (Math.abs(delta) < 0.001) {
			anim.playbackRate = target;
			onDone?.();
			return;
		}

		// ease-out for spin-up (delta > 0): t² flipped — fast start, slow finish.
		// ease-in for spin-down (delta < 0): t² — slow start, fast finish.
		const ease = delta > 0 ? (t: number) => 1 - (1 - t) * (1 - t) : (t: number) => t * t;

		let startTime: number | null = null;

		function step(now: number) {
			if (startTime === null) startTime = now;
			const t = Math.min((now - startTime) / durationMs, 1);
			anim.playbackRate = start + delta * ease(t);
			if (t < 1) {
				rampRafId = requestAnimationFrame(step);
			} else {
				rampRafId = null;
				onDone?.();
			}
		}

		rampRafId = requestAnimationFrame(step);
	}

	$effect(() => {
		if (!discEl) return;

		const anim = getAnim();

		if (isStarting) {
			anim.play();
			rampRate(anim, 0.33, 800);
		} else if (isRunning) {
			anim.play();
			rampRate(anim, 1, 600);
		} else if (isSpinningDown) {
			rampRate(anim, 0, 1500, () => anim.pause());
		} else if (resolvedStatus === 'stopped') {
			rampRate(anim, 0, 800, () => anim.pause());
		} else {
			// dry_run_lock — hard snap, no ramp.
			if (rampRafId !== null) {
				cancelAnimationFrame(rampRafId);
				rampRafId = null;
			}
			anim.pause();
		}

		return () => {
			if (rampRafId !== null) {
				cancelAnimationFrame(rampRafId);
				rampRafId = null;
			}
		};
	});
</script>

<div class="rounded-[1.75rem] border border-slate-200 bg-slate-50 p-4 shadow-sm">
	<div class="flex flex-col items-center gap-3">
		<div class="relative flex h-29 w-29 items-center justify-center">
			<div
				class="absolute inset-0 rounded-full border-[6px] bg-white shadow-inner transition-colors duration-500 {ringClass}"
			>
				<!--
					bind:this wires the DOM element to discEl so the $effect above
					can call .animate() on it directly via WAAPI.
				-->
				<div
					bind:this={discEl}
					class="absolute inset-[0.62rem] rounded-full border border-white/35 shadow-[inset_0_1px_0_rgba(255,255,255,0.35)]"
					style="background: {discGradient}; transition: background 500ms ease;"
				>
					<div
						class="absolute inset-0 rounded-full transition-[background] duration-500"
						style="background: {discSheen};"
					></div>

					<Aperture
						size={80}
						strokeWidth={2}
						class="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 rotate-[14deg] drop-shadow-[0_1px_1px_rgba(255,255,255,0.12)] transition-colors duration-500 {bladeClass}"
					/>

					<div
						class="absolute top-1/2 left-1/2 h-[2.5rem] w-[2.5rem] -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/60 shadow-[0_2px_7px_rgba(15,23,42,0.18)] transition-colors duration-500 {hubClass}"
					></div>

					<div
						class="absolute top-1/2 left-1/2 h-6 w-6 -translate-x-1/2 -translate-y-1/2 rounded-full border border-slate-950/10 bg-slate-950"
					></div>
				</div>

				{#if isLocked}
					<div class="bg-black-950/25 absolute inset-[0.62rem] rounded-full backdrop-blur-[3px]">
						<div class="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-4/7">
							<StatusIcon
								size={50}
								class={`drop-shadow-[0_1px_1px_rgba(255,255,255,0.25)] ${iconClass}`}
							/>
						</div>
					</div>
				{/if}

				{#if isNull}
					<div class="bg-black-950/25 absolute inset-[0.62rem] rounded-full backdrop-blur-[3px]">
						<div class="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2">
							<LoaderCircle
								size={70}
								class={`animate-spin drop-shadow-[0_1px_1px_rgba(255,255,255,0.25)] ${iconClass}`}
							/>
						</div>
					</div>
				{/if}
			</div>

			{#if isStarting}
				<div
					class="absolute inset-0 animate-ping rounded-full border-4 border-amber-400 opacity-60"
				></div>
			{/if}
		</div>

		<div class="w-full text-center">
			<p class="text-xs font-semibold tracking-[0.2em] text-slate-500 uppercase">{label}</p>
			<div
				class="mt-1 flex flex-col items-center justify-center gap-3 sm:flex-row sm:justify-between"
			>
				<p class="text-xl font-semibold uppercase transition-colors duration-500 {labelClass}">
					{runtimeStatusLabels[resolvedStatus]}
				</p>
				{#if resolvedStatus === 'dry_run_lock' && onUnlock}
					<button
						type="button"
						class="mt-1 shrink-0 rounded-full border border-rose-200 bg-rose-50 px-3 py-1 text-sm font-semibold tracking-[0.14em] text-rose-700 uppercase shadow-sm transition hover:bg-rose-100 active:scale-[0.98] disabled:pointer-events-none disabled:cursor-not-allowed disabled:border-slate-200 disabled:bg-slate-100 disabled:text-slate-400 disabled:shadow-none"
						disabled={unlockDisabled}
						onclick={onUnlock}
					>
						Unlock
					</button>
				{/if}
			</div>
		</div>
	</div>
</div>
