import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
	appId: 'com.app.neptune',
	appName: 'Neptune',
	webDir: 'build',
	ios: {
		contentInset: 'always'
	},
	android: {
		allowMixedContent: false,
		captureInput: true,
		webContentsDebuggingEnabled: false
	},
	plugins: {
		StatusBar: {
			style: 'default',
			overlaysWebView: false
		}
	}
};

export default config;
