<script setup>
const emit = defineEmits([ 'load' ])
const { loading, data, refresh } = useFetch('/api/schedules')
emit('load', refresh)

const headers = [
	{
		title: 'Name',
		key: 'name',
	},
	{
		title: 'Aktionen',
		key: 'actions',
	},
]
</script>

<template lang="pug">
v-data-table(
	v-if="!loading && data"
	:headers="headers"
	:items="data"
	item-value="name"
	disable-sort
	no-filter
	no-data-text="Keine Schedules vorhanden"
	items-per-page-text="Einträge pro Seite:"
	:items-per-page="10"
	:items-per-page-options="[{ title: '10', value: 10 }]"
	page-text=""
	show-current-page
)
	template(#item.actions="{ item }")
		FileDeleteDialog(:name="item.name" collection="schedules" @refresh="refresh")
		ActionDialog(
			:path="`/api/schedules/${item.name}`"
			:title="`Schedule: ${item.name}`"
			label=""
			:buttonProps="{ icon: 'mdi-magnify', variant: 'flat', size: 'small' }"
		)
		v-btn(
			variant="flat"
			icon="mdi-chart-bar"
			@click="navigateTo(`/render/${item.name}`)"
		)
v-progress-linear(
	indeterminate
	v-else
)
</template>
