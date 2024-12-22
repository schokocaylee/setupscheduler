import { readdir } from 'node:fs/promises'
const config = useRuntimeConfig()

export default defineEventHandler(async event => {
	if(!config.dataPath) {
		throw new Error('Missing data path')
	}
	const files = await readdir(config.dataPath)
	return files
		.filter(name => !name.startsWith('.'))
		.filter(name => name.endsWith('.ins.json'))
		.map(name => ({ name: name.substring(0, name.length - '.ins.json'.length) }))
})
