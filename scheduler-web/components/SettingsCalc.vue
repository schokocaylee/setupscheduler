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

const searchModes = {
	none: { binary: false, jump: false },
	binary: { binary: true, jump: false },
	jump: { binary: false, jump: true },
	safe: { binary: true, jump: true },
}
function updateSearchMode(key, value) {
	const newMode = {
		...searchModes[model.value.searchMode],
		[key]: value,
	}
	model.value.searchMode = Object.entries(searchModes)
		.find(mode => Object.keys(newMode)
			.every(key => mode[1][key] === newMode[key]))
		?.[0]
}
const binarySearch = computed({
	get: () => searchModes[model.value.searchMode]?.binary || false,
	set: binary => updateSearchMode('binary', binary),
})
const classJump = computed({
	get: () => searchModes[model.value.searchMode]?.jump || false,
	set: jump => updateSearchMode('jump', jump),
})
const setMakespan = computed({
	get: () => model.value.searchMode === 'makespan',
	set: value => {
		model.value.searchMode = value ? 'makespan' : 'jump'
	},
})

const schedulers = [
	{ value: 'split', title: 'Paralleles Scheduling' },
	{ value: 'preempt', title: 'Präemptives Scheduling' },
	{ value: 'nonpreempt', title: 'Nicht-präemptives Scheduling' },
]

watch(() => model.value.scheduler, value => {
	if(value === 'nonpreempt') {
		model.value.searchMode = 'binary'
	}
}, { immediate: true })
</script>

<template lang="pug">
v-card(height="100%")
	v-card-title Berechnung
	v-card-subtitle Einstellungen für die Berechnung
	v-card-text
		v-select(
			id="scheduler"
			v-model="model.scheduler"
			label="Scheduler"
			permanent-label
			:items="schedulers"
		)
		v-divider.my-4
		v-checkbox(
			:readonly="model.scheduler === 'nonpreempt'"
			id="setMakespan"
			v-model="setMakespan"
			color="primary"
			label="Makespan direkt festlegen"
		)
		template(v-if="setMakespan")
			v-text-field(
				id="makespan"
				type="number"
				:min="0"
				v-model="model.makespan"
				label="Makespan"
			)
		template(v-else)
			v-divider.mb-4
			.text-h6.text-center Suchverfahren
			v-switch(
				:readonly="model.scheduler === 'nonpreempt'"
				id="classJump"
				color="primary"
				v-model="classJump"
				label="Class Jumping"
				hide-details
			)
			v-switch(
				:readonly="model.scheduler === 'nonpreempt'"
				id="binarySearch"
				color="primary"
				v-model="binarySearch"
				label="Binäre Suche"
				hide-details
			)
</template>
