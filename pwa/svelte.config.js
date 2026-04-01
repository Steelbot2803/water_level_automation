import adapterNetlify from '@sveltejs/adapter-netlify';
import adapterVercel from '@sveltejs/adapter-vercel';

/** @type {import('@sveltejs/kit').Config} */

function getAdapter() {
	const explicitAdapter = process.env.ADAPTER;
	if (explicitAdapter) {
		switch (explicitAdapter) {
			case 'netlify':
				return adapterNetlify();
			case 'vercel':
				return adapterVercel();
			default:
				throw new Error(`Unknown adapter: ${explicitAdapter}`);
		}
	}

	if (process.env.NETLIFY) {
		return adapterNetlify();
	}

	if (process.env.VERCEL) {
		return adapterVercel();
	}

	// Keep Vercel as a local/default fallback when no deployment platform is detected.
	return adapterNetlify();
}

export default {
	kit: {
		adapter: getAdapter(),
		serviceWorker: {
			register: true,
			files: (filepath) => {
				if (
					filepath.endsWith('.map') ||
					filepath.includes('hot-update') ||
					filepath.includes('dev')
				)
					return false;
				return true;
			}
		}
	}
};
