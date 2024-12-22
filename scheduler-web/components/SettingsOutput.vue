<script setup>
const props = defineProps({
	modelValue: {
		type: Object,
	},
})
const emit = defineEmits([ 'update:modelValue' ])
const model = computed({
	get: () => props.modelValue,
	set: value => {
		emit('update:modelValue', value)
	},
})

const printSchedule = computed({
	get: () => model.value.printMode === 'schedule',
	set: value => {
		model.value.printMode = value ? 'schedule' : 'instance'
	},
})
const printInstance = computed({
	get: () => model.value.printMode !== 'none',
	set: value => {
		model.value.printMode = value ? 'instance' : 'none'
	},
})
</script>

<template lang="pug">
v-card(height="100%")
	v-card-title Schedule
	v-card-subtitle Ausgabe konfigurieren
	v-card-text
		v-switch(
			id="printSchedule"
			color="primary"
			v-model="printSchedule"
			label="Ausgabe des Schedules erzeugen"
			hide-details
		)
		v-switch(
			id="printInstance"
			:disabled="printSchedule"
			color="primary"
			v-model="printInstance"
			label="Ausgabe der Instanz erzeugen"
			hide-details
		)
		v-divider.my-4
		v-switch(
			id="validate"
			color="primary"
			v-model="model.validate"
			label="Schedule validieren"
			hide-details
		)
</template>
