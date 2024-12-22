#include "generate.h"
#include <stdlib.h>
#include <error.h>
#include <sys/param.h>
#include <math.h>

uint32_t rand_interval(uint32_t min, uint32_t max) {
	if(max < min) {
		error(1, 0, "Random interval invalid");
		exit(1);
	}
	int r;
	uint32_t range = 1 + max - min;
	uint32_t buckets = RAND_MAX / range;
	uint32_t limit = buckets * range;
	do { r = rand(); } while(r >= limit);
	return min + (r / buckets);
}

uint32_t rand_interval_reduced(uint32_t min, uint32_t max, uint32_t reducer) {
	uint32_t center = MAX(min, MIN(max, (uint32_t)((double)(max - min) / reducer + min + 0.5)));
	uint32_t vmax = 2;
	uint32_t c2 = 2*center - 1*min; if(c2 < max) vmax++;
	uint32_t c3 = 3*center - 2*min; if(c3 < max) vmax++;
	uint32_t c4 = 4*center - 3*min; if(c4 < max) vmax++;
	uint32_t variant = rand_interval(1, vmax);
	switch(variant) {
		case 1: return rand_interval(min, center);
		case 2: return rand_interval(center, max);
		case 3: return rand_interval(center, c2);
		case 4: return rand_interval(center, c3);
		case 5: return rand_interval(center, c4);
	}
	return rand_interval(min, max);
}

instance_t* generate_instance(int64_t machines, int64_t classes, int64_t makespan) {
	instance_t* instance = malloc(sizeof(instance_t));
	instance->machines = machines;
	instance->classCount = classes;
	instance->classes = calloc(instance->classCount, sizeof(instance_class_t));
	instance->jobCount = 0;

	int64_t budget = makespan * instance->machines * 2;
	int64_t i;
	for(i = 0; i < instance->classCount && budget > 0; i++) {
		int64_t minLoad = 1, maxLoad = 3.3 * makespan;
		int64_t minDuration = 1, maxDuration = makespan - 1;

		instance_class_t* class = &instance->classes[i];
		int variant = rand_interval(1, 5);
		switch(variant) {
			case 1: // E+
				class->setup = rand_interval(1./2. * makespan + 1, MIN(makespan - 1, MAX(1./2. * makespan + 2, .8 * makespan)));
				maxDuration = .7 * (makespan - class->setup);
				minLoad = makespan - class->setup;
				maxLoad = 3.3 * (makespan - class->setup);
				break;
			case 2: // E0
				class->setup = rand_interval(1./2. * makespan + 1, MIN(makespan - 2, MAX(1./2. * makespan + 2, .8 * makespan)));
				maxDuration = .7 * (makespan - class->setup);
				minLoad = MAX(3./4. * makespan - class->setup + 1, 1);
				maxLoad = makespan - class->setup - 1;
				break;
			case 3: // E-
				class->setup = rand_interval(1./2. * makespan + 1, 3./4. * makespan - 1);
				maxDuration = .7 * (makespan - class->setup);
				maxLoad = 3./4. * makespan - class->setup;
				break;
			case 4: // C+
				class->setup = rand_interval(1./4. * makespan, 1./2. * makespan);
				maxDuration = .9 * (makespan - class->setup);
				maxLoad = 3.3 * (makespan - class->setup);
				break;
			case 5: // C-
				class->setup = rand_interval(1, 1./4. * makespan - 1);
				maxDuration = .9 * (makespan - class->setup);
				maxLoad = 3.3 * (makespan - class->setup);
				switch(rand_interval(1, 6)) {
					case 1: // C-
						maxDuration = 1./2. * makespan - class->setup;
						break;
					default: // C*
						minDuration = 1./3. * makespan - class->setup;
						minLoad = 1.5 * (makespan - class->setup);
						break;
				}
				break;
		}

		int64_t minJobs = minLoad / maxDuration + 1;
		int64_t maxJobs = maxLoad / minDuration;
		class->jobCount = rand_interval(minJobs, MIN(maxJobs, MAX(3 * minJobs, maxJobs / class->setup)));
		class->jobs = calloc(class->jobCount, sizeof(instance_job_t));

		int64_t runtime = 0;
		int64_t j;
		for(j = 0; j < class->jobCount && budget > 0; j++) {
			int64_t remain = class->jobCount - j - 1;
			int64_t minimal = MAX(minDuration, minLoad - runtime - remain * maxDuration);
			int64_t maximal = MIN(maxDuration, maxLoad - runtime - remain * minDuration);

			instance_job_t* job = &class->jobs[j];
			job->duration = rand_interval_reduced(minimal, maximal, class->jobCount / 3);
			runtime += job->duration;
		}
		class->jobCount = j;
		instance->jobCount += class->jobCount;
		budget -= runtime + class->setup * MAX(1, runtime / (makespan - class->setup));
	}
	instance->classCount = i;

	return instance;
}
