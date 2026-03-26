<svelte:options runes={true} />

<script lang="ts">
	import { onDestroy } from 'svelte';
	import jsQR from 'jsqr';
	import { decodeBrokerSettingsFromQR } from '$lib/control.js';
	import type { BrokerSettings } from '$lib/types.js';

	let {
		onResult,
		onClose,
		inline = false,
		timeoutMs = 12000
	}: {
		onResult: (settings: Partial<BrokerSettings>) => void;
		onClose: () => void;
		inline?: boolean;
		// How long to wait before flashing the border to signal "nothing found yet"
		timeoutMs?: number;
	} = $props();

	let videoEl = $state<HTMLVideoElement | null>(null);
	let canvasEl = $state<HTMLCanvasElement | null>(null);
	let error = $state<string | null>(null);
	let scanning = $state(false);
	let borderFlash = $state(false);

	let rafId: number | null = null;
	let stream: MediaStream | null = null;
	let timeoutId: ReturnType<typeof setTimeout> | null = null;

	$effect(() => {
		if (!videoEl) return;
		startCamera();
		return () => stopAll();
	});

	async function startCamera() {
		error = null;
		try {
			stream = await navigator.mediaDevices.getUserMedia({
				video: { facingMode: 'environment' }
			});
			videoEl!.srcObject = stream;
			await videoEl!.play();
			scanning = true;
			scheduleFrame();
			armTimeout();
		} catch (e) {
			error =
				e instanceof DOMException && e.name === 'NotAllowedError'
					? 'Camera permission denied. Allow camera access and try again.'
					: 'Could not open camera.';
		}
	}

	// Arms the "nothing found" timeout. Called on start and reset after each flash
	// so the user gets another full window on every attempt.
	function armTimeout() {
		if (timeoutId !== null) clearTimeout(timeoutId);
		timeoutId = setTimeout(() => {
			// Flash the reticle border amber for 600 ms then restore it.
			// Does NOT close the scanner — user may want to try again immediately.
			borderFlash = true;
			setTimeout(() => {
				borderFlash = false;
				armTimeout(); // re-arm for another cycle
			}, 600);
		}, timeoutMs);
	}

	function stopAll() {
		if (rafId !== null) {
			cancelAnimationFrame(rafId);
			rafId = null;
		}
		if (timeoutId !== null) {
			clearTimeout(timeoutId);
			timeoutId = null;
		}
		stream?.getTracks().forEach((t) => t.stop());
		stream = null;
		scanning = false;
	}

	function scheduleFrame() {
		rafId = requestAnimationFrame(scanFrame);
	}

	function scanFrame() {
		const video = videoEl;
		const canvas = canvasEl;
		if (!video || !canvas || video.readyState < video.HAVE_ENOUGH_DATA) {
			scheduleFrame();
			return;
		}

		const ctx = canvas.getContext('2d', { willReadFrequently: true });
		if (!ctx) {
			scheduleFrame();
			return;
		}

		canvas.width = video.videoWidth;
		canvas.height = video.videoHeight;
		ctx.drawImage(video, 0, 0);

		const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
		const result = jsQR(imageData.data, imageData.width, imageData.height);

		if (result) {
			const settings = decodeBrokerSettingsFromQR(result.data);
			if (settings) {
				stopAll();
				onResult(settings);
				return;
			}
			// QR found but not Neptune format — keep scanning, reset timeout
			armTimeout();
		}

		scheduleFrame();
	}

	onDestroy(stopAll);
</script>

<div
	class={inline
		? 'relative h-full w-full overflow-hidden rounded-2xl bg-white'
		: 'fixed inset-0 z-50 flex flex-col bg-white'}
>
	{#if error}
		<div class="flex h-full items-center justify-center px-6">
			<p class="text-center text-sm text-rose-300">{error}</p>
		</div>
	{:else}
		<!-- svelte-ignore a11y_media_has_caption -->
		<video bind:this={videoEl} class="hidden" playsinline></video>
		<canvas bind:this={canvasEl} class="h-full w-full object-cover"></canvas>

		<div class="pointer-events-none absolute inset-0 flex items-center justify-center">
			<!--
				borderFlash transitions the border from white to amber.
				CSS transition handles the colour change smoothly — no JS animation loop needed.
			-->
			<div
				class={`mt-4 h-2/3 w-2/3 rounded-xl border-3 shadow-[0_0_0_9999px_rgba(255,255,255,0.45)]
					transition-colors duration-150
					${borderFlash ? 'border-amber-400' : 'border-black/70'}`}
			></div>
		</div>

		{#if scanning}
			<p
				class="absolute right-0 bottom-3 left-0 text-center text-[0.65rem] tracking-widest text-black/75 uppercase"
			>
				{borderFlash ? 'No valid QR found — try again' : 'Point at a Neptune QR'}
			</p>
		{/if}
	{/if}
</div>
