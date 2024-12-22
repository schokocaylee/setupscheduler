import { z } from 'zod'
const schema = z.object({
	input: z.object({
		mode: z.enum([ 'generate', 'generateExpert', 'file' ]),
		factor: z.number().int().positive().finite().safe(),
		factorMachines: z.number().int().positive().finite().safe(),
		factorClasses: z.number().int().positive().finite().safe(),
		factorMakespan: z.number().int().positive().finite().safe(),
		file: z.string().regex(/^[a-zA-Z0-9._-]*$/),
	}).strict(),
	calc: z.object({
		scheduler: z.enum([ 'split', 'preempt', 'nonpreempt' ]),
		searchMode: z.enum([ 'none', 'makespan', 'jump', 'binary', 'safe' ]),
		makespan: z.number().positive().finite().safe(),
	}).strict(),
	output: z.object({
		printMode: z.enum([ 'schedule', 'instance', 'none' ]),
		validate: z.boolean(),
	}).strict(),
}).strict().deepPartial()

export default defineEventHandler(async event => {
	const body = await readValidatedBody(event, schema.parse)
	return await runScheduler(body)
})
