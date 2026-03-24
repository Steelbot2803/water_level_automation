import adapterNetlify from '@sveltejs/adapter-netlify';
import adapterVercel from '@sveltejs/adapter-vercel';

/** @type {import('@sveltejs/kit').Config} */

function getAdapter() {
	const adapter = process.env.ADAPTER || 'vercel';
	switch (adapter) {
		case 'netlify':
			return adapterNetlify();
		case 'vercel':
			return adapterVercel();
		default:
			throw new Error(`Unknown adapter: ${adapter}`);
	}
}

export default {
	kit: {
		adapter: getAdapter()
	}
};
