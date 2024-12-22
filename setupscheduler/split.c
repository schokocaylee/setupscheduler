#include "schedule.h"
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>

int64_t schedule_expensive_get_machine_count(instance_t* instance, instance_class_t* class) {
	return ceil(2. * class->load / instance->makespan);
}

static void schedule_expensive(instance_t* instance, schedule_t* schedule, wrap_position_t* wraps) {
	int64_t m = 0, c = -1; double t = 0;
	for(int64_t i = 0, e = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_wrap_sequence(schedule, &m, &t, &c, class->setup, class->setup + 1./2. * instance->makespan, job->duration, class->setup, i, j);
		}
		wraps[e].machine = m;
		wraps[e].lower = t + 1./2. * instance->makespan;
		wraps[e].upper = 3./2. * instance->makespan;
		e++; m++; t = 0;
	}
}

static bool schedule_cheap(instance_t* instance, schedule_t* schedule, wrap_position_t* wraps, int64_t wrapCount) {
	int64_t wi = 0, c = -1; double wt = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_wrap_position_sequence(schedule, wraps, &wi, &wt, &c, job->duration, class->setup, i, j);
			if(wi >= wrapCount) return false;
		}
	}
	return true;
}

bool schedule_split_lsplit(instance_t* instance, double* ptr) {
	double lsplit = instance->load;
	int64_t mexp = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(CHECK(class->category, EXPENSIVE)) {
			int64_t beta = schedule_expensive_get_machine_count(instance, class);
			lsplit += beta * class->setup;
			mexp += beta;
		} else {
			lsplit += class->setup;
		}
	}
	if(mexp > instance->machines) {
		if(ptr) *ptr = -INFINITY;
		return false;
	}
	if(ptr) *ptr = lsplit;
	return lsplit <= instance->machines * instance->makespan;
}
inline bool test_schedule_split(instance_t* instance) {
	return schedule_split_lsplit(instance, NULL);
}

bool schedule_split(instance_t* instance, schedule_t* schedule) {
	int64_t mexp = 0, exp = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(CHECK(class->category, EXPENSIVE)) {
			mexp += schedule_expensive_get_machine_count(instance, class);
			exp++;
		}
	}
	if(mexp > instance->machines) return false;

	int64_t wrapCount = exp + (instance->machines - mexp);
	wrap_position_t* wraps = calloc(wrapCount + 1, sizeof(wrap_position_t));
	schedule_expensive(instance, schedule, wraps);
	for(int64_t i = mexp; i < instance->machines; i++) {
		wrap_position_t* pos = &wraps[i - mexp + exp];
		pos->machine = i;
		pos->lower = 1./2. * instance->makespan;
		pos->upper = 3./2. * instance->makespan;
	}
	wrap_position_t* pos = &wraps[wrapCount];
	pos->machine = instance->machines;
	pos->lower = 0;
	pos->upper = 3./2. * instance->makespan;

	bool result = schedule_cheap(instance, schedule, wraps, wrapCount);
	free(wraps);
	return result;
}
