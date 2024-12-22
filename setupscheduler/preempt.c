#include "schedule.h"
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>

static void schedule_expensive_zero(instance_t* instance, schedule_t* schedule, int64_t m) {
	double t = 1./2. * instance->makespan; int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE_ZERO)) continue;
		t += class->setup;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_schedule_item(schedule, m, t, job->duration, i, j);
			t += job->duration;
		}
		m++; t = 1./2. * instance->makespan;
	}
}

int64_t schedule_expensive_plus_get_machine_count(instance_t* instance, instance_class_t* class) {
	double beta = 2 * class->load / instance->makespan;
	int64_t betaprime = beta;
	if(class->load - betaprime * instance->makespan / 2. <= instance->makespan - class->setup) {
		return MAX(betaprime, 1);
	}
	return ceil(beta);
}

static void schedule_expensive_plus(instance_t* instance, schedule_t* schedule, int64_t m) {
	double t = 0; int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE_PLUS)) continue;
		int64_t mstart = m;
		int64_t mend = mstart + schedule_expensive_plus_get_machine_count(instance, class) - 1;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			double upper = m >= mend ? (3./2. * instance->makespan) : (class->setup + 1./2. * instance->makespan);
			add_wrap_sequence(schedule, &m, &t, &c, class->setup, upper, job->duration, class->setup, i, j);
		}
		m++; t = 0;
	}
}

static void schedule_expensive_minus(instance_t* instance, schedule_t* schedule, int64_t m) {
	double t = 0; int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE_MINUS)) continue;
		t += class->setup;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_schedule_item(schedule, m, t, job->duration, i, j);
			t += job->duration;
		}
		if(t <= 3./4. * instance->makespan) {
			t = 3./4. * instance->makespan;
		} else {
			m++; t = 0;
		}
	}
}

static void schedule_cheap_plus(instance_t* instance, schedule_t* schedule, int64_t* m, double* t) {
	int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP_PLUS)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_wrap_sequence(schedule, m, t, &c, 1./2. * instance->makespan, 3./2. * instance->makespan, job->duration, class->setup, i, j);
		}
	}
}

static double cks_calc(instance_class_t* class) {
	return class->setup / (class->load - class->starredLoad);
}
static int cks_compar(const void* a, const void* b, void* c) {
	int64_t i = *(const int64_t*)a, j = *(const int64_t*)b;
	instance_t* instance = (instance_t*)c;
	double diff = cks_calc(&instance->classes[j]) - cks_calc(&instance->classes[i]);
	return diff == 0 ? 0 : (diff > 0 ? 1 : -1);
}
static double schedule_continuous_knapsack_starred(instance_t* instance, schedule_t* schedule, int64_t m, double t, double capacity) {
	int64_t* cks = calloc(instance->classCount, sizeof(int64_t));
	int64_t star = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		if(CHECK(instance->classes[i].category, STAR)) cks[star++] = i;
	}
	qsort_r(cks, star, sizeof(int64_t), cks_compar, instance);

	int64_t c = -1;
	double largeSetups = 0.;
	for(int64_t k = 0; k < star; k++) {
		int64_t i = cks[k];
		instance_class_t* class = &instance->classes[i];
		double weight = class->load - class->starredLoad;
		if(capacity >= weight) {
			class->splitMode = 1.;
			capacity -= weight;
		} else if(capacity > 0.) {
			class->category |= SPLIT;
			class->splitMode = capacity / weight;
			capacity = 0.;
		} else {
			class->splitMode = 0.;
			largeSetups += class->setup;
		}
		if(schedule) {
			for(int64_t j = 0; j < class->jobCount; j++) {
				instance_job_t* job = &class->jobs[j];
				double duration = CHECK(job->category, STAR)
					? (class->setup + job->duration - 1./2. * instance->makespan) + (1./2. * instance->makespan - class->setup) * class->splitMode
					: job->duration * class->splitMode;
				add_wrap_sequence(schedule, &m, &t, &c, 1./2. * instance->makespan, 3./2. * instance->makespan, duration, class->setup, i, j);
			}
		}
	}
	free(cks);
	return largeSetups;
}

static void schedule_starred_and_greedy_cheap_minus(instance_t* instance, schedule_t* schedule, int64_t m, double t) {
	int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, STAR)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_wrap_sequence(schedule, &m, &t, &c, 1./2. * instance->makespan, 3./2. * instance->makespan, job->duration, class->setup, i, j);
		}
	}
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if((class->category & (CHEAP_MINUS | STAR)) != CHEAP_MINUS) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			add_wrap_sequence(schedule, &m, &t, &c, 1./2. * instance->makespan, 3./2. * instance->makespan, job->duration, class->setup, i, j);
			if(m >= instance->machines) {
				remove_last_schedule_item(schedule, i, j);
				return;
			}
		}
	}
}

static bool schedule_remaining_on_large(instance_t* instance, schedule_t* schedule, int64_t m, int64_t m2) {
	double t = 0;
	int64_t c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		schedule_class_t* classSchedule = &schedule->classes[i];
		if(!CHECK(class->category, SPLIT)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			schedule_job_t* jobSchedule = &classSchedule->jobs[j];
			double remain = job->duration - jobSchedule->scheduledDuration;
			if(remain <= 0) continue;
			if(m > m2) return false;
			if(remain > 1./4. * instance->makespan) continue;
			add_wrap_sequence(schedule, &m, &t, &c, class->setup, 1./2. * instance->makespan, remain, class->setup, i, j);
		}
	}
	if(t > 1./4. * instance->makespan) {
		m++;
	}
	t = 1./4. * instance->makespan;
	c = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		schedule_class_t* classSchedule = &schedule->classes[i];
		if(!CHECK(class->category, CHEAP_MINUS)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			schedule_job_t* jobSchedule = &classSchedule->jobs[j];
			double remain = job->duration - jobSchedule->scheduledDuration;
			if(remain <= 0) continue;
			if(m > m2) return false;
			if(remain > 1./4. * instance->makespan) {
				add_schedule_item(schedule, m2--, class->setup, remain, i, j);
			} else {
				add_wrap_sequence(schedule, &m, &t, &c, 1./4. * instance->makespan, 1./2. * instance->makespan, remain, class->setup, i, j);
			}
		}
	}
	return m <= m2 || (m <= m2 + 1 && c < 0);
}

bool schedule_preempt_lpmtn(instance_t* instance, double* ptr) {
	double lpmtn = instance->load;
	double capacity = instance->machines * instance->makespan;
	int64_t mprime = 0; bool mprimehalf = false;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(CHECK(class->category, EXPENSIVE_PLUS)) {
			int64_t gamma = schedule_expensive_plus_get_machine_count(instance, class);
			lpmtn += gamma * class->setup;
			capacity -= gamma * class->setup + class->load;
			mprime += gamma;
		} else if(CHECK(class->category, EXPENSIVE_ZERO)) {
			lpmtn += class->setup;
			capacity -= instance->makespan;
			mprime++;
		} else if(CHECK(class->category, EXPENSIVE_MINUS)) {
			lpmtn += class->setup;
			capacity -= class->effectiveLoad;
			if((mprimehalf = !mprimehalf)) mprime++;
		} else if(CHECK(class->category, CHEAP_MINUS)) {
			lpmtn += class->setup;
			if(CHECK(class->category, STAR)) {
				capacity -= class->setup + class->starredLoad;
			}
		} else {
			lpmtn += class->setup;
			capacity -= class->effectiveLoad;
		}
	}
	if(instance->machines < mprime) {
		if(ptr) *ptr = -INFINITY;
		return false;
	}
	if(capacity < 0) {
		if(ptr) *ptr = capacity;
		return false;
	}
	lpmtn += schedule_continuous_knapsack_starred(instance, NULL, 0, 0, MAX(capacity, 0));
	if(ptr) *ptr = lpmtn;
	return lpmtn <= instance->machines * instance->makespan;
}
inline bool test_schedule_preempt(instance_t* instance) {
	return schedule_preempt_lpmtn(instance, NULL);
}

bool schedule_preempt(instance_t* instance, schedule_t* schedule) {
	int64_t plus = 0, minus = 0, zero = 0;
	double starredClassLoad = 0, starredLoad = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(CHECK(class->category, EXPENSIVE_PLUS)) {
			plus += schedule_expensive_plus_get_machine_count(instance, class);
		} else if(CHECK(class->category, EXPENSIVE_MINUS)) {
			minus++;
		} else if(CHECK(class->category, EXPENSIVE_ZERO)) {
			zero++;
		} else if(CHECK(class->category, STAR)) {
			starredClassLoad += class->effectiveLoad;
			starredLoad += class->setup + class->starredLoad;
		}
	}
	if(zero + plus + (minus+1)/2 > instance->machines) {
		return false;
	}

	schedule_expensive_zero(instance, schedule, 0);
	schedule_expensive_plus(instance, schedule, zero);
	schedule_expensive_minus(instance, schedule, zero + plus);

	int64_t m = zero + plus + minus/2;
	double t = (minus%2 ? 1. : 1./2.) * instance->makespan;
	schedule_cheap_plus(instance, schedule, &m, &t);
	if(m > instance->machines) {
		return false;
	}

	double freeOut = (instance->machines - m + 1./2.) * instance->makespan - t;
	if(freeOut < starredClassLoad) {
		if(freeOut < starredLoad) {
			return false;
		}
		schedule->category |= PREEMPT_KNAPSACK;
		schedule_continuous_knapsack_starred(instance, schedule, m, t, freeOut - starredLoad);
	} else {
		schedule_starred_and_greedy_cheap_minus(instance, schedule, m, t);
	}

	return schedule_remaining_on_large(instance, schedule, 0, zero - 1);
}
