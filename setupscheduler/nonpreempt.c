#include "schedule.h"
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>
#include <string.h>

static int64_t schedule_get_machine_count(instance_t* instance, instance_class_t* class) {
	if(CHECK(class->category, EXPENSIVE)) {
		return ceil(class->load / (instance->makespan - class->setup));
	}
	int64_t m = 0;
	double kload = 0.;
	for(int64_t i = 0; i < class->jobCount; i++) {
		instance_job_t* job = &class->jobs[i];
		if(job->duration > 1./2. * instance->makespan) {
			m++;
		} else if(class->setup + job->duration > 1./2. * instance->makespan) {
			kload += job->duration;
		}
	}
	return m + ceil(kload / (instance->makespan - class->setup));
}

bool schedule_nonpreempt_lnonp(instance_t* instance, double* ptr) {
	double lnonp = instance->load;
	int64_t mprime = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		int64_t m = schedule_get_machine_count(instance, class);
		mprime += m;
		lnonp += m * class->setup;
		if(class->load - m * (instance->makespan - class->setup) > 0) {
			lnonp += class->setup;
		}
	}
	if(mprime > instance->machines) {
		if(ptr) *ptr = -INFINITY;
		return false;
	}
	if(ptr) *ptr = lnonp;
	return lnonp <= instance->machines * instance->makespan;
}
inline bool test_schedule_nonpreempt(instance_t* instance) {
	return schedule_nonpreempt_lnonp(instance, NULL);
}



static void add_wrap_position(wrap_position_t** wraps, int64_t* wrapCount, int64_t i, int64_t machine, double lower, double upper) {
	wraps[i] = reallocarray(wraps[i], wrapCount[i] + 1, sizeof(wrap_position_t));
	wrap_position_t* pos = &wraps[i][wrapCount[i]];
	pos->machine = machine;
	pos->lower = lower;
	pos->upper = upper;
	wrapCount[i]++;
}

static void schedule_nonpreempt_expensive(instance_t* instance, schedule_t* schedule, int64_t* m, wrap_position_t** globalWraps, int64_t* globalWrapCount) {
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE)) continue;
		double t = class->setup;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_schedule_item(schedule, *m, t, job->duration, i, j);
			t += job->duration;
			if(t > instance->makespan) {
				(*m)++;
				t = class->setup;
			}
		}
		if(t > class->setup) {
			add_wrap_position(globalWraps, globalWrapCount, 0, *m, t, instance->makespan);
			(*m)++;
		}
	}
}

static void schedule_nonpreempt_jplus(instance_t* instance, schedule_t* schedule, int64_t* m, wrap_position_t** wraps, int64_t* wrapCount) {
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			if(job->duration <= 1./2. * instance->makespan) continue;
			add_schedule_item(schedule, *m, class->setup, job->duration, i, j);
			add_wrap_position(wraps, wrapCount, i, *m, class->setup + job->duration, instance->makespan);
			(*m)++;
		}
	}
}

static void schedule_nonpreempt_k(instance_t* instance, schedule_t* schedule, int64_t* m, wrap_position_t** wraps, int64_t* wrapCount) {
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP)) continue;
		bool hasJob = false;
		double t = class->setup;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			if(job->duration > 1./2. * instance->makespan || class->setup + job->duration <= 1./2. * instance->makespan) continue;
			add_schedule_item(schedule, *m, t, job->duration, i, j);
			t += job->duration;
			if(t > instance->makespan) {
				(*m)++;
				t = class->setup;
			}
			hasJob = true;
		}
		if(hasJob && t > class->setup) {
			add_wrap_position(wraps, wrapCount, i, *m, t, instance->makespan);
			(*m)++;
		}
	}
}

static void schedule_nonpreempt_remaining(instance_t* instance, schedule_t* schedule, wrap_position_t** wraps, int64_t* wrapCount, wrap_position_t** globalWraps, int64_t* globalWrapCount) {
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP) || wrapCount[i] == 0) continue;
		int64_t wi = 0;
		double wt = 0.;
		for(int64_t j = 0; j < class->jobCount && wi < wrapCount[i]; j++) {
			instance_job_t* job = &class->jobs[j];
			if(class->setup + job->duration > 1./2. * instance->makespan) continue;
			wrap_position_t* wrap = &wraps[i][wi];
			add_schedule_item(schedule, wrap->machine, wrap->lower + wt, job->duration, i, j);
			wt += job->duration;
			if(wt > wrap->upper - wrap->lower) {
				wi++;
				wt = 0.;
			}
		}
		for(; wi < wrapCount[i]; wi++) {
			wrap_position_t* wrap = &wraps[i][wi];
			add_wrap_position(globalWraps, globalWrapCount, 0, wrap->machine, wrap->lower + wt, wrap->upper);
			wt = 0.;
		}
	}
}

static void schedule_nonpreempt_new_setups(instance_t* instance, schedule_t* schedule, wrap_position_t* globalWraps, int64_t globalWrapCount) {
	int64_t wi = 0; double wt = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		schedule_class_t* classSchedule = &schedule->classes[i];
		if(!CHECK(class->category, CHEAP)) continue;
		bool hasJobs = false;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			schedule_job_t* jobSchedule = &classSchedule->jobs[j];
			if(jobSchedule->scheduledDuration > 0) continue;
			hasJobs = true;
			break;
		}
		if(!hasJobs) continue;
		bool hasSetup = false;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			schedule_job_t* jobSchedule = &classSchedule->jobs[j];
			if(jobSchedule->scheduledDuration > 0) continue;
			wrap_position_t* wrap = &globalWraps[wi];
			if(!hasSetup) {
				wt += class->setup;
				hasSetup = true;
			}
			add_schedule_item(schedule, wrap->machine, wrap->lower + wt, job->duration, i, j);
			wt += job->duration;
			if(wt > wrap->upper - wrap->lower) {
				wi++;
				wt = 0.;
				hasSetup = false;
			}
		}
	}
}

bool schedule_nonpreempt(instance_t* instance, schedule_t* schedule) {
	wrap_position_t** wraps = calloc(instance->classCount, sizeof(wrap_position_t*));
	int64_t* wrapCount = calloc(instance->classCount, sizeof(int64_t));
	wrap_position_t* globalWraps = NULL;
	int64_t globalWrapCount = 0;
	int64_t m = 0;

	schedule_nonpreempt_expensive(instance, schedule, &m, &globalWraps, &globalWrapCount);
	schedule_nonpreempt_jplus(instance, schedule, &m, wraps, wrapCount);
	schedule_nonpreempt_k(instance, schedule, &m, wraps, wrapCount);

	schedule_nonpreempt_remaining(instance, schedule, wraps, wrapCount, &globalWraps, &globalWrapCount);
	for(; m < instance->machines; m++) {
		add_wrap_position(&globalWraps, &globalWrapCount, 0, m, 0, instance->makespan);
	}

	schedule_nonpreempt_new_setups(instance, schedule, globalWraps, globalWrapCount);

	for(int64_t i = 0; i < instance->classCount; i++) {
		free(wraps[i]);
	}
	free(wraps);
	free(wrapCount);
	free(globalWraps);
	return true;
}
