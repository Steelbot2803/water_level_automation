import { mdsvex } from 'mdsvex';
import adapter from '@sveltejs/adapter-netlify';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	kit: { adapter: adapter() },
	preprocess: [mdsvex()],
	extensions: ['.svelte', '.svx']
};

export default config;
