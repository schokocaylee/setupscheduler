import { readdir } from 'node:fs/promises'
const config = useRuntimeConfig()

export default defineEventHandler(async event => {
	if(!config.dataPath) {
		throw new Error('Missing data path')
	}
	const files = await readdir(config.dataPath)
	return files
		.filter(name => !name.startsWith('.'))
		.filter(name => name.endsWith('.sch.json'))
		.map(name => ({ name: name.substring(0, name.length - '.sch.json'.length) }))
})
