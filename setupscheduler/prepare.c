#include "prepare.h"
#include <sys/param.h>

void prepare_instance(instance_t* instance, double makespan) {
	instance->makespan = makespan;
	instance->load = 0;
	instance->effectiveLoad = 0;
	instance->longestEffectiveJob = -1;

	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		class->load = 0;
		class->category = 0;
		class->splitMode = 0;
		class->starredLoad = 0;
		class->longestEffectiveJob = -1;

		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			job->category = 0;
			class->load += job->duration;
			if(class->setup + job->duration > class->longestEffectiveJob) {
				class->longestEffectiveJob = class->setup + job->duration;
				if(class->longestEffectiveJob > instance->longestEffectiveJob) {
					instance->longestEffectiveJob = class->longestEffectiveJob;
				}
			}
		}

		class->effectiveLoad = class->setup + class->load;
		instance->load += class->load;
		instance->effectiveLoad += class->effectiveLoad;

		if(class->setup > makespan/2.) {
			if(class->effectiveLoad >= makespan) {
				class->category |= EXPENSIVE_PLUS;
			} else if(class->effectiveLoad > makespan*3./4.) {
				class->category |= EXPENSIVE_ZERO;
			} else {
				class->category |= EXPENSIVE_MINUS;
			}
		} else {
			if(class->setup >= makespan/4.) {
				class->category |= CHEAP_PLUS;
			} else {
				class->category |= CHEAP_MINUS;
				for(int64_t j = 0; j < class->jobCount; j++) {
					instance_job_t* job = &class->jobs[j];
					if(class->setup + job->duration > makespan/2.) {
						job->category |= STAR;
						class->category |= STAR;
						class->starredLoad += class->setup + job->duration - makespan/2.;
					}
				}
			}
		}
	}

	instance->lowerMakespan = MAX(instance->effectiveLoad / instance->machines, instance->longestEffectiveJob);
	instance->upperMakespan = instance->effectiveLoad;
}
