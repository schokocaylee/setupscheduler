import { promisify } from 'node:util'
import { access } from 'node:fs/promises'
import path from 'node:path'
import { execFile as execFileCallback } from 'node:child_process'

const config = useRuntimeConfig()
const execFile = promisify(execFileCallback)

export async function executeScheduler(...params) {
	if(!config.schedulerCommand || !config.dataPath) {
		throw new Error('Missing scheduler')
	}
	try {
		const { stdout, stderr } = await execFile(
			config.schedulerCommand,
			params,
			{
				cwd: config.dataPath,
				env: {},
			}
		)
		return { stdout, stderr }
	} catch(e) {
		throw createError({
			status: 422,
			message: 'Scheduler failed',
			data: e.toString(),
		})
	}
}

function getISOTime() {
	return new Date().toISOString().replaceAll(/[TZ:.-]/g, '')
}

export async function runScheduler(options) {
	const params = []
	switch(options.input.mode) {
		case 'generate':
			params.push(`--generate=${options.input.factor}`)
			break
		case 'generateExpert':
			params.push(`--generate=${options.input.factor}`)
			params.push(`--factor-machine=${options.input.factorMachines}`)
			params.push(`--factor-class=${options.input.factorClasses}`)
			params.push(`--factor-makespan=${options.input.factorMakespan}`)
			break
		case 'file':
			const filename = `${options.input.file}.ins.json`
			await access(path.join(config.dataPath, filename))
			params.push(`--instance=${filename}`)
			break
	}
	switch(options.calc.scheduler) {
		case 'split':
			params.push('--split')
			break
		case 'preempt':
			params.push('--preempt')
			break
		case 'nonpreempt':
			params.push('--nonpreempt')
			break
	}
	switch(options.calc.searchMode) {
		case 'none':
			params.push('--skip-calc')
			break
		case 'makespan':
			params.push(`--makespan=${options.calc.makespan}`)
			break
		case 'jump':
			break
		case 'binary':
			params.push('--binary')
			break
		case 'safe':
			params.push('--safe')
			break
	}
	switch(options.output.printMode) {
		case 'instance':
			if(options.input.mode === 'generate') {
				const outputInstanceFilename = `random-${getISOTime()}.ins.json`
				params.push(`--output=${outputInstanceFilename}`)
			}
			params.push('--skip-schedule')
			break
		case 'schedule':
			const outputFilename = (options.input.mode === 'file' ? options.input.file : `random-${getISOTime()}`) + '.sch.json'
			params.push(`--output=${outputFilename}`)
			break
		case 'none':
			params.push('--no-output')
			break
	}
	if(options.output.validate) {
		params.push('--validate')
	}
	params.push('--analyze')
	const result = await executeScheduler(...params)
	return result.stderr
}
