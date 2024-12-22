<script setup>
const settings = inject('settings')

const sizeIcon = computed(() => ({
	small: 'mdi-size-s',
	medium: 'mdi-size-m',
	large: 'mdi-size-l',
}[settings.value.size]))
function nextSize() {
	const sizes = ['small', 'medium', 'large']
	const nextIndex = (sizes.indexOf(settings.value.size) + 1) % sizes.length
	settings.value.size = sizes[nextIndex]
}
</script>

<template lang="pug">
v-app
	v-app-bar
		template(#prepend)
			v-app-bar-nav-icon(icon="mdi-home" @click="navigateTo('/')")
		v-app-bar-title Scheduling mit Klassenrüstzeiten
		template(#append)
			template(v-if="$route.path.startsWith('/render/')")
				v-btn(
					:icon="sizeIcon"
					@click="nextSize()"
				)
	v-main.ma-4
		slot
</template>
