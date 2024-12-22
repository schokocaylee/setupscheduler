<script setup>
const props = defineProps({
	schedule: {
		type: Object,
		required: true,
	},
})
const settings = inject('settings')
const mainClass = computed(() => ({ [settings.value.size]: true }))
const schedule = computed(() => props.schedule)
const render = computed(() => schedule.value.makespan * 3/2)

const scales = [1, 2, 3, 4, 5].map(i => ({
	...(i % 2 === 0 ? { 'data-scale-makespan-halfs': i/2 } : {}),
	'data-scale-makespan-quarters': i,
	style: {
		'z-index': 1,
		bottom: (100. * i/4 * schedule.value.makespan / render.value) + '%',
	},
}))

const list = computed(() => schedule.value.classes
	.flatMap((cl, i) => cl.jobs.flatMap(({ schedule }, j) => (schedule ?? []).flatMap(it => ({ i, j, ...it })))))
const machines = computed(() => [...Array(schedule.value.machines).keys()])
const jobs = computed(() => machines.value.map(k => list.value.filter(({ machine }) => k === machine)))
const setups = computed(() => machines.value.map(k => jobs.value[k]
	.sort(({ time: a }, { time: b }) => a - b)
	.reduce(({ c, setups }, { i, time }) => {
		if(c !== i) {
			const setup = schedule.value.classes[i].setup
			setups.push({ setup, start: time - setup, i })
		}
		return { c: i, setups }
	}, { c: -1, setups: [] })
	.setups
))
</script>

<template lang="pug">
#maincontainer(:class="mainClass")
	#main
		.scales
			template(v-for="scale of scales")
				.scale(v-bind="scale")
		template(v-for="machine of machines")
			.machine(:data-machine="machine")
				template(v-for="{ i, j, time, len } of jobs[machine]")
					.job(
						:data-job="`${i},${j}`"
						:data-class-type="schedule.classes[i].type"
						:data-star="schedule.classes[i].jobs[j].star"
						:data-start-time="time"
						:data-duration="len"
						:style="{ bottom: (100 * time / render) + '%', height: (100 * len / render) + '%' }"
					)
				template(v-for="{ setup, start, i } of setups[machine]")
					.setup(
						:data-class="i"
						:data-type="schedule.classes[i].type"
						:data-star="schedule.classes[i].star"
						:data-start-time="start"
						:data-duration="setup"
						:style="{ bottom: (100 * start / render) + '%', height: (100 * setup / render) + '%' }"
					)
</template>

<style scoped lang="scss">
#maincontainer {
	position: relative;
	margin: 0;
	padding: 0;
	height: 100%;
	overflow: auto;

	& > #main {
		border: 1px solid black;
		box-sizing: border-box;

		position: absolute;
		top: 0;
		left: 0;
		min-width: 100%;
		bottom: 0;

		display: flex;
		flex-direction: row;
	}

	.scales {
		position: absolute;
		left: 0;
		right: 0;
		top: 0;
		bottom: 0;
	}
	.scale {
		position: absolute;
		left: 0;
		right: 0;
		border: 1px dotted black;
	}
	.scale[data-scale-makespan-halfs] {
		border-style: dashed;
	}

	.machine {
		flex: 1;
		min-width: 0.1em;
		position: relative;
	}

	.job {
		position: absolute;
		display: block;
		left: 0;
		right: 0;
		border-bottom: 1px solid gray;
	}

	.setup {
		position: absolute;
		display: block;
		left: 0;
		right: 0;
	}

	&.small {
		& > #main {
			padding-bottom: 2em;
		}

		.scales {
			bottom: 2em;
		}

		.machine {
			margin: 0.3em;
			min-width: 2em;
			border: 1px solid black;
			&::after {
				content: 'M' attr(data-machine);
				position: absolute;
				display: block;
				top: calc(100% + 0.5em);
				width: 100%;
				text-align: center;
			}
		}

		.job {
			background-color: rgb(255, 0, 0, 0.5);
			&::after { content: attr(data-job); }
		}

		.setup {
			background-color: rgba(0, 0, 255, 0.5);
			&::after { content: attr(data-class); }
		}
	}

	&.medium {
		.machine {
			min-width: 0.8em;
			border-right: 0.3px solid black;
		}
	}

	&.medium, &.large {
		.setup {
			background-color: rgba(0, 0, 0, 0.5);
			&[data-type="E+"] { background-color: rgba(0, 100, 255, 1); }
			&[data-type="E0"] { background-color: rgba(0, 200, 200, 1); }
			&[data-type="E-"] { background-color: rgba(0, 255, 100, 1); }
			&[data-type="C+"] { background-color: rgba(255, 0, 0, 1); }
			&[data-type="C-"] { background-color: rgba(255, 100, 0, 1); }
			&[data-star="true"] { background-color: rgba(255, 150, 0, 1); }
		}
		.job {
			background-color: rgb(0, 0, 0, 0.3);
			&[data-class-type="E+"] { background-color: rgba(0, 100, 255, 0.5); }
			&[data-class-type="E0"] { background-color: rgba(0, 200, 200, 0.5); }
			&[data-class-type="E-"] { background-color: rgba(0, 255, 100, 0.5); }
			&[data-class-type="C+"] { background-color: rgba(255, 0, 0, 0.5); }
			&[data-class-type="C-"] { background-color: rgba(255, 100, 0, 0.5); }
			&[data-star="true"] { background-color: rgba(255, 200, 0, 0.5); }
		}
	}
}
</style>
