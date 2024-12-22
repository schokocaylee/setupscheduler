export default defineEventHandler(async event => {
	return (await executeScheduler('--help')).stdout
})
