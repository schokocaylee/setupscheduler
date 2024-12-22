<script setup>
const params = ref({
	input: {
		mode: 'file',
		factor: 1,
		factorMachines: '1',
		factorClasses: '1',
		factorMakespan: '10',
		file: '',
	},
	calc: {
		searchMode: 'jump',
		scheduler: 'split',
		makespan: '1',
	},
	output: {
		printMode: 'schedule',
		validate: true,
	},
})
const sendParams = computed(() => ({
	input: {
		...params.value.input,
		factorMachines: Number(params.value.input.factorMachines),
		factorClasses: Number(params.value.input.factorClasses),
		factorMakespan: Number(params.value.input.factorMakespan),
	},
	calc: {
		...params.value.calc,
		makespan: Number(params.value.calc.makespan),
	},
	output: {
		...params.value.output,
	},
}))

const paramsValid = computed(() => {
	switch(params.value.input.mode) {
		case 'file': return !!params.value.input.file
	}
	return true
})

const refreshSchedules = ref(() => { /* */ })
</script>

<template lang="pug">
v-form
	v-container
		v-row(align-content="stretch")
			v-col(lg="4" cols="12")
				SettingsInput(
					v-model="params.input"
				)
			v-col(lg="4" cols="12")
				SettingsCalc(
					v-model="params.calc"
				)
			v-col(lg="4" cols="12")
				SettingsOutput(
					v-model="params.output"
				)
	v-card.ma-8
		v-card-title Scheduler aufrufen
		v-card-subtitle Aktionen ausführen und Ergebnisse betrachten
		v-card-text
			ActionDialog(
				path="/api/scheduler"
				:params="{ method: 'POST', body: sendParams }"
				title="Ergebnis des Schedulers"
				label="Scheduler starten"
				:buttonProps="{ prependIcon: 'mdi-rocket', color: 'primary', disabled: !paramsValid }"
				@action="refreshSchedules()"
			)
			ActionDialog(
				path="/api/help"
				title="Hilfetext"
				label="Hilfe"
			)
			ActionDialog(
				path="/api/version"
				title="Versionsinformationen"
				label="Version"
			)
	v-card.ma-8
		v-card-title Visualisierung
		v-card-subtitle Erzeugte Schedules betrachten
		v-card-text
			ScheduleList(@load="refreshSchedules = $event")
</template>
