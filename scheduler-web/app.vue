<script setup>
const settings = ref({
	size: 'medium',
})
provide('settings', settings)
</script>

<template lang="pug">
NuxtLayout
	NuxtPage
</template>
