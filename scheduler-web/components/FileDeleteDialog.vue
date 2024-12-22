<script setup>
const props = defineProps({
	name: {
		type: String,
		required: true,
	},
	collection: {
		type: String,
		default: 'instances',
	},
})
const emit = defineEmits([ 'refresh' ])

async function run() {
	await $fetch(`/api/${props.collection}/${props.name}`, { method: 'DELETE' })
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
			icon="mdi-delete"
		)
	template(#default="{ isActive }")
		v-card(title="Datei löschen")
			v-card-text Wollen Sie die Datei wirklich löschen?
			v-card-actions
				v-btn(
					text="Nein, abbrechen"
					@click="isActive.value = false"
				)
				v-btn(
					color="red"
					text="Ja, löschen"
					@click="run().then(() => { isActive.value = false })"
				)
</template>
