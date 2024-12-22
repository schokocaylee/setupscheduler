// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
	compatibilityDate: '2024-11-01',
	devtools: { enabled: true },
	modules: [ 'vuetify-nuxt-module' ],
	runtimeConfig: {
		schedulerCommand: '../../setupscheduler/setupscheduler',
		dataPath: './data',
	},
	vite: {
		css: {
			preprocessorOptions: {
				scss: {
					api: 'modern-compiler',
				},
			},
		},
	},
	ssr: false, // https://github.com/vuetifyjs/vuetify/issues/19015
})
