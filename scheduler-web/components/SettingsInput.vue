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

const factors = [1, 10, 100, 1000, 10000, 100000, 1000000]
const factorLabels = computed(() => Object.fromEntries(Object.entries(factors).map(([k, v]) => ([k, `10^${Math.round(Math.log(v) / Math.log(10))}`]))))
const factor = computed({
	get: () => {
		const idx = factors.indexOf(model.value.factor)
		return idx >= 0 ? idx : 0
	},
	set: index => {
		model.value.factor = factors[index]
	},
})
</script>

<template lang="pug">
v-card(height="100%")
	v-card-title Instanz
	v-card-subtitle Instanz hochladen oder generieren
	v-card-text
		v-radio-group(v-model="model.mode")
			v-radio(
				value="file"
				label="Instanz wählen"
			)
			v-radio(
				value="generate"
				label="Instanz zufällig generieren"
			)
			v-radio(
				value="generateExpert"
				label="Instanz zufällig generieren (Expertenmodus)"
			)
		v-divider.mb-4
		template(v-if="model.mode === 'file'")
			FileChooser(v-model="model.file")
		template(v-if="model.mode === 'generate'")
			.text-h6.text-center Größenfaktor
			v-slider(
				v-model="factor"
				:step="1"
				show-ticks="always"
				:tick-size="4"
				:ticks="factorLabels"
				:max="factors.length - 1"
			)
		template(v-if="model.mode === 'generateExpert'")
			.text-h6.text-center.mb-4 Skalierung
			v-text-field(
				id="factor-machines"
				type="number"
				:min="1"
				v-model="model.factorMachines"
				label="Maschinen"
			)
			v-text-field(
				id="factor-classes"
				type="number"
				:min="1"
				v-model="model.factorClasses"
				label="Klassen"
			)
			v-text-field(
				id="factor-makespan"
				type="number"
				:min="10"
				v-model="model.factorMakespan"
				label="Angestrebter Makespan"
			)
</template>
