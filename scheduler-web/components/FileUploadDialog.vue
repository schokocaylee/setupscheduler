<script setup>
const emit = defineEmits([ 'refresh' ])

async function run(file) {
	const body = new FormData()
	body.append('file', file)
	await $fetch('/api/instances', { method: 'POST', body })
	emit('refresh')
}
</script>

<template lang="pug">
v-dialog(:max-width="512")
	template(#activator="{ props: activatorProps }")
		v-btn(
			v-bind="activatorProps"
			size="small"
			variant="flat"
			icon="mdi-upload"
		)
	template(#default="{ isActive }")
		v-card(title="Datei hochladen")
			v-card-text
				v-file-input(
					label="Instanzdatei"
					accept=".ins.json"
					@update:modelValue="run($event).then(() => { isActive.value = false })"
				)
			v-card-actions
				v-btn(
					text="abbrechen"
					@click="isActive.value = false"
				)
</template>
