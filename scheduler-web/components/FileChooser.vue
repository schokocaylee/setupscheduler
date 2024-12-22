<script setup>
const props = defineProps({
	modelValue: {
		type: String,
	},
})
const emit = defineEmits([ 'update:modelValue' ])
const selected = computed({
	get: () => [ props.modelValue ],
	set: ([ value ]) => {
		emit('update:modelValue', value)
	},
})

const { loading, data, refresh } = useFetch('/api/instances')

const headers = [
	{
		title: 'Name',
		key: 'name',
	},
	{
		title: '',
		key: 'actions',
	},
]
</script>

<template lang="pug">
v-data-table(
	v-if="!loading && data"
	v-model="selected"
	:headers="headers"
	:items="data"
	item-value="name"
	show-select
	select-strategy="single"
	disable-sort
	no-filter
	no-data-text="Keine Instanzen vorhanden"
	items-per-page-text="Einträge pro Seite:"
	:items-per-page="5"
	:items-per-page-options="[{ title: '5', value: 5 }]"
	page-text=""
	show-current-page
)
	template(#header.actions="{}")
		FileUploadDialog(@refresh="refresh")
	template(#item.actions="{ item }")
		FileDeleteDialog(:name="item.name" @refresh="refresh")
		ActionDialog(
			:path="`/api/instances/${item.name}`"
			:title="`Instanz: ${item.name}`"
			label=""
			:buttonProps="{ icon: 'mdi-magnify', variant: 'flat', size: 'small' }"
		)
v-progress-linear(
	indeterminate
	v-else
)
</template>
