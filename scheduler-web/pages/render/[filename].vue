<script setup>
const route = useRoute()
const { loading, data } = useFetch(`/api/schedules/${route.params.filename}`)
</script>

<template lang="pug">
#container
	DrawSchedule(
		v-if="!loading && data"
		:schedule="data"
	)
	v-progress-linear(
		v-else
		indeterminate
	)
</template>

<style scoped lang="scss">
#container {
	position: relative;
	height: 100%;
}
</style>
