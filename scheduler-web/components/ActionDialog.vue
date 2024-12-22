<script setup>
const props = defineProps({
	path: {
		type: String,
		required: true,
	},
	params: {
		type: Object,
		default: () => ({ method: 'GET' }),
	},
	title: {
		type: String,
		required: true,
	},
	label: {
		type: String,
		required: true,
	},
	buttonProps: {
		type: Object,
		default: () => ({ prependIcon: 'mdi-information' }),
	},
})
const emit = defineEmits([ 'action' ])

const data = ref(null)
const isError = ref(false)
async function load() {
	try {
		data.value = await $fetch(props.path, props.params)
		emit('action')
	} catch(e) {
		isError.value = true
		data.value = e.data?.message + '\n\n' + e.data?.data
	}
}
</script>

<template lang="pug">
v-dialog(:max-width="1024" @afterEnter="load" @afterLeave="data = null; isError = false" persistent)
	template(#activator="{ props: activatorProps }")
		v-btn.ml-2(
			v-bind="{ ...activatorProps, ...props.buttonProps }"
			:text="props.label"
		)
	template(#default="{ isActive }")
		v-card(:title="props.title")
			v-card-text
				pre(v-if="data" :style="isError ? { color: 'red' } : {}") {{ data }}
				v-progress-linear(v-else indeterminate)
			v-card-actions
				v-btn(
					v-if="data"
					text="schließen"
					@click="isActive.value = false"
				)
</template>

<style scoped lang="scss">
pre {
	white-space: pre-wrap;
	overflow: auto;
	height: 60vh;
}
</style>
