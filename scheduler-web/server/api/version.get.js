export default defineEventHandler(async event => {
	return (await executeScheduler('--version')).stdout
})
