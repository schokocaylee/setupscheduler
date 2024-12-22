import { writeFile } from 'node:fs/promises'
import path from 'node:path'
const config = useRuntimeConfig()

export default defineEventHandler(async event => {
	if(!config.dataPath) {
		throw new Error('Missing data path')
	}
	const data = await readMultipartFormData(event)
	const file = data.find(item => item.name === 'file')
	if(!file) {
		throw new Error('Missing file')
	}
	if(file.type !== 'application/json') {
		throw new Error('Bad file type')
	}
	if(!/^[a-zA-Z0-9_.-]+\.ins\.json$/.test(file.filename)) {
		throw new Error('Bad filename')
	}
	await writeFile(path.join(config.dataPath, file.filename), file.data)
	sendNoContent(event)
})
