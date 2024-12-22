import { access, unlink } from 'node:fs/promises'
import path from 'node:path'
const config = useRuntimeConfig()

export default defineEventHandler(async event => {
	if(!config.dataPath) {
		throw new Error('Missing data path')
	}
	const baseFilename = getRouterParam(event, 'filename')
	if(!/^[a-zA-Z0-9._-]+$/.test(baseFilename)) {
		throw new Error('Bad filename')
	}
	const filename = path.join(config.dataPath, `${baseFilename}.ins.json`)
	await access(filename)
	await unlink(filename)
	sendNoContent(event)
})
