#include "search.h"
#include "basic.h"
#include "schedule.h"
#include "prepare.h"
#include "test.h"
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/param.h>

static int64_t binary(int64_t lower, int64_t upper, instance_t* instance, scheduletester_t scheduler) {
	while(lower <= upper) {
		int64_t center = (lower + upper) / 2;
		if(testBinary(center, instance, scheduler)) {
			upper = center - 1;
		} else {
			lower = center + 1;
		}
	}
	return lower;
}

int64_t scheduler_binarysearch(instance_t* instance, scheduletester_t scheduler) {
	prepare_instance(instance, 1);
	return binary(instance->lowerMakespan, instance->upperMakespan, instance, scheduler);
}

schedule_t* scheduler_makespan(instance_t* instance, scheduler_t scheduler, double makespan) {
	prepare_instance(instance, makespan);
	if(makespan < (instance->load / instance->machines) || makespan < instance->longestEffectiveJob) {
		return NULL;
	}
	schedule_t* schedule = create_schedule(instance);
	if(!scheduler(instance, schedule)) {
		delete_schedule(schedule);
		return NULL;
	}
	return schedule;
}
