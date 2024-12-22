#include "jump.h"
#include "basic.h"
#include "prepare.h"
#include "schedule.h"
#include "test.h"
#include <stdlib.h>
#include <math.h>
#include <sys/param.h>

static int64_t find_right_interval(double* jumps, int64_t count, instance_t* instance, scheduletester_t tester) {
	int64_t lower = 0;
	int64_t upper = count - 1;
	while(lower <= upper) {
		int64_t center = (lower + upper) / 2;
		if(testBinary(jumps[center], instance, tester)) {
			upper = center - 1;
		} else {
			lower = center + 1;
		}
	}
	return lower;
}

static int compar_double(const void* a, const void* b) {
	double x = *(double*)a;
	double y = *(double*)b;
	return x < y ? -1 : (x > y ? 1 : 0);
}

static void step2(instance_t* instance, double* a1, double* t1) {
	double* ljumps = calloc(4 * instance->classCount, sizeof(double));
	double* ljump = ljumps;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];

		double expMax = class->setup * 2;
		*ljump++ = expMax;

		double plusMax = class->effectiveLoad;
		if(plusMax < expMax) {
			*ljump++ = plusMax;

			double zeroMax = 4./3. * class->effectiveLoad;
			if(zeroMax < expMax) {
				*ljump++ = zeroMax;
			}
		}

		double cPlusMax = class->setup * 4;
		*ljump++ = cPlusMax;
	}
	int64_t jumpCount = ljump - ljumps;
	qsort(ljumps, jumpCount, sizeof(double), compar_double);
	int64_t jumpIdx = find_right_interval(ljumps, jumpCount, instance, test_schedule_preempt);
	*t1 = jumpIdx == jumpCount ? INFINITY : ljumps[jumpIdx];
	*a1 = jumpIdx == 0 ? 0 : ljumps[jumpIdx - 1];
	free(ljumps);
}

static int64_t step3(instance_t* instance) {
	int64_t f = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE_PLUS)) continue;
		if(f >= 0 && instance->classes[f].effectiveLoad >= class->effectiveLoad) continue;
		f = i;
	}
	return f;
}

static void step4(instance_t* instance, instance_class_t* class, double* a2, double* t2) {
	int64_t gamma = schedule_expensive_plus_get_machine_count(instance, class);
	int64_t jumpCount = instance->machines + 1;
	double* jumps = calloc(jumpCount, sizeof(double));
	double* jump = jumps;
	for(int64_t k = instance->machines; k >= 0; k--, jump++) {
		*jump = 2. * class->effectiveLoad / (gamma + k + 2);
	}
	int64_t jumpIdx = find_right_interval(jumps, jumpCount, instance, test_schedule_preempt);
	*t2 = jumpIdx == jumpCount ? INFINITY : jumps[jumpIdx];
	*a2 = jumpIdx == 0 ? 0 : jumps[jumpIdx - 1];
	free(jumps);
}

static void step6(instance_t* instance, int64_t f, double a3, double t3, double* a4, double* t4) {
	double* jumps = calloc(instance->classCount, sizeof(double));
	double* jump = jumps;
	for(int64_t i = 0; i < instance->classCount; i++) {
		if(i == f) continue;
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE_PLUS)) continue;
		int64_t gamma = schedule_expensive_plus_get_machine_count(instance, class);
		for(int64_t k1 = 0, k2 = instance->machines; k1 <= k2; ) {
			int64_t k = (k1 + k2) / 2;
			double t = 2. * class->effectiveLoad / (gamma + k + 2);
			if(t <= a3) {
				k2 = k - 1;
			} else if(t > t3) {
				k1 = k + 1;
			} else {
				*jump++ = t;
				break;
			}
		}
	}
	int64_t jumpCount = jump - jumps;
	if(jumpCount == 0) {
		*t4 = t3;
		*a4 = a3;
	} else {
		qsort(jumps, jumpCount, sizeof(double), compar_double);
		int64_t jumpIdx = find_right_interval(jumps, jumpCount, instance, test_schedule_preempt);
		*t4 = jumpIdx == jumpCount ? t3 : jumps[jumpIdx];
		*a4 = jumpIdx == 0 ? a3 : jumps[jumpIdx - 1];
	}
	free(jumps);
}

static double step7(instance_t* instance, double a4, double t4) {
	int64_t zero = 0;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(CHECK(class->category, EXPENSIVE_ZERO)) {
			zero++;
		}
	}

	double* kjumps = calloc(instance->jobCount, sizeof(double));
	double* kjump = kjumps;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, CHEAP_MINUS)) continue;
		if(CHECK(class->category, STAR)) continue;
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			double t = job->duration * 4.;
			if(t > a4 && t < t4) {
				*kjump++ = t;
			}
			double tlarge = (class->setup + job->duration) * 2.;
			if(tlarge > a4 && tlarge < t4) {
				*kjump++ = tlarge;
			}
		}
	}
	int64_t jumpCount = kjump - kjumps;
	int64_t jumpIdx = 0;
	if(jumpCount > 0) {
		qsort(kjumps, jumpCount, sizeof(double), compar_double);
		jumpIdx = find_right_interval(kjumps, jumpCount, instance, test_schedule_preempt);
		if(jumpIdx < jumpCount && kjumps[jumpIdx] < t4) {
			t4 = kjumps[jumpIdx];
		}
		if(jumpIdx > 0 && kjumps[jumpIdx - 1] > a4) {
			a4 = kjumps[jumpIdx - 1];
		}
	}
	free(kjumps);

	double lpmtnfail;
	prepare_instance(instance, t4 - EPSILON_JUMP);
	schedule_preempt_lpmtn(instance, &lpmtnfail);

	if(lpmtnfail >= 0) {
		double tnew = lpmtnfail / instance->machines + EPSILON_JUMP;
		if(tnew > a4 && tnew < t4) {
			prepare_instance(instance, tnew);
			t4 = tnew;

			prepare_instance(instance, t4);
			schedule_preempt_lpmtn(instance, &lpmtnfail);
			tnew = lpmtnfail / instance->machines + EPSILON_JUMP;
			if(tnew > a4 && tnew < t4) {
				prepare_instance(instance, tnew);
				if(test_schedule_preempt(instance)) t4 = tnew;
			}
		}
	}

	prepare_instance(instance, a4 + 2*EPSILON_JUMP);
	if(test_schedule_preempt(instance)) {
		return instance->makespan;
	}

	prepare_instance(instance, t4 - 2*EPSILON_JUMP);
	if(!test_schedule_preempt(instance)) {
		return t4;
	}

	return t4;
}

double preempt_classjump(instance_t* instance) {
	prepare_instance(instance, 1);

	double a1, t1;
	step2(instance, &a1, &t1);
	prepare_instance(instance, t1 - EPSILON);

	int64_t f = step3(instance);
	if(f >= 0) {
		double a2, t2;
		step4(instance, &instance->classes[f], &a2, &t2);
		double a3 = MAX(instance->lowerMakespan - EPSILON_JUMP, MAX(a1, a2)), t3 = MIN(instance->upperMakespan, MIN(t1, t2));
		prepare_instance(instance, t3 - EPSILON);

		double a4, t4;
		step6(instance, f, a3, t3, &a4, &t4);
		prepare_instance(instance, t4);

		double t = step7(instance, a4, t4);
		return t;
	}
	return t1;
}

static int64_t split_find_fastest(instance_t* instance) {
	int64_t f = -1;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE)) continue;
		if(f >= 0 && instance->classes[f].load >= class->load) continue;
		f = i;
	}
	return f;
}

static void split_guess_interval(instance_t* instance, instance_class_t* class, double* a2, double* t2) {
	int64_t beta = schedule_expensive_get_machine_count(instance, class);
	int64_t jumpCount = instance->machines + 1;
	double* jumps = calloc(jumpCount, sizeof(double));
	double* jump = jumps;
	for(int64_t k = instance->machines; k >= 0; k--, jump++) {
		*jump = 2. * class->load / (beta + k);
	}
	int64_t jumpIdx = find_right_interval(jumps, jumpCount, instance, test_schedule_split);
	*t2 = jumpIdx == jumpCount ? INFINITY : jumps[jumpIdx];
	*a2 = jumpIdx == 0 ? 0 : jumps[jumpIdx - 1];
	free(jumps);
}

static void split_check_jumps(instance_t* instance, int64_t f, double a3, double t3, double* a4, double* t4) {
	double* jumps = calloc(instance->classCount, sizeof(double));
	double* jump = jumps;
	for(int64_t i = 0; i < instance->classCount; i++) {
		if(i == f) continue;
		instance_class_t* class = &instance->classes[i];
		if(!CHECK(class->category, EXPENSIVE)) continue;
		int64_t beta = schedule_expensive_get_machine_count(instance, class);
		double t = 2. * class->load / beta;
		if(t < a3) continue;
		*jump++ = t;
	}
	int64_t jumpCount = jump - jumps;
	if(jumpCount == 0) {
		*t4 = t3;
		*a4 = a3;
	} else {
		qsort(jumps, jumpCount, sizeof(double), compar_double);
		int64_t jumpIdx = find_right_interval(jumps, jumpCount, instance, test_schedule_split);
		*t4 = jumpIdx == jumpCount ? t3 : jumps[jumpIdx];
		*a4 = jumpIdx == 0 ? a3 : jumps[jumpIdx - 1];
	}
	free(jumps);
}

double split_classjump(instance_t* instance) {
	prepare_instance(instance, 1);

	double* stilde = calloc(instance->classCount + 2, sizeof(double));
	stilde[instance->classCount + 1] = instance->effectiveLoad;
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		stilde[i + 1] = 2. * class->setup;
	}
	qsort(stilde + 1, instance->classCount, sizeof(double), compar_double);
	int64_t idx = find_right_interval(stilde, instance->classCount + 2, instance, test_schedule_split);
	double t1 = stilde[idx], a1 = stilde[idx - 1];
	prepare_instance(instance, t1 - EPSILON);

	int64_t f = split_find_fastest(instance);
	if(f >= 0) {
		double a2, t2;
		split_guess_interval(instance, &instance->classes[f], &a2, &t2);

		double a3 = MAX(instance->lowerMakespan - EPSILON_JUMP, MAX(a1, a2)), t3 = MIN(instance->upperMakespan, MIN(t1, t2));
		prepare_instance(instance, t3 - EPSILON);

		double a4, t4;
		split_check_jumps(instance, f, a3, t3, &a4, &t4);

		prepare_instance(instance, a4 + EPSILON);
		double lsplitfail;
		schedule_split_lsplit(instance, &lsplitfail);
		if(lsplitfail < 0) {
			return t4;
		}
		double tnew = lsplitfail / instance->machines;
		return MIN(t4, tnew);
	}
	return instance->effectiveLoad / instance->machines;
}
