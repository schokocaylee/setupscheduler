#include "validate.h"
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

static void validate_indices_and_sums(instance_t* instance, schedule_t* schedule) {
	if(instance->classCount != schedule->classCount) {
		error(1, 0, "Invalid class count %ld in schedule (should be %ld)", schedule->classCount, instance->classCount);
	}
	int64_t jobCount = 0;
	for(int64_t i = 0; i < schedule->classCount; i++) {
		schedule_class_t* class = &schedule->classes[i];
		instance_class_t* instanceClass = &instance->classes[i];
		jobCount += class->jobCount;
		if(instanceClass->jobCount != class->jobCount) {
			error(1, 0, "Invalid job count %ld in class %ld in schedule (should be %ld)", class->jobCount, i, instanceClass->jobCount);
		}
		for(int64_t j = 0; j < class->jobCount; j++) {
			schedule_job_t* job = &class->jobs[j];
			instance_job_t* instanceJob = &instanceClass->jobs[j];
			if(instanceJob->duration != job->scheduledDuration) {
				error(1, 0, "Invalid scheduled duration %lf in job %ld,%ld in schedule (should be %lf by instance)", job->scheduledDuration, i, j, instanceJob->duration);
			}
			double duration = 0;
			for(int64_t k = 0; k < job->itemCount; k++) {
				schedule_item_t* item = &job->items[k];
				if(item->machine < 0 || item->machine >= instance->machines) {
					error(1, 0, "Invalid machine %ld in job item %ld,%ld,%ld in schedule (instance has %ld machines)", item->machine, i, j, k, instance->machines);
				}
				if(item->time < 0) {
					error(1, 0, "Invalid start time %lf in job item %ld,%ld,%ld in schedule", item->time, i, j, k);
				}
				duration += item->duration;
			}
			if(job->scheduledDuration != duration) {
				error(1, 0, "Invalid scheduled duration items %lf in job %ld,%ld in schedule (should be %lf)", duration, i, j, job->scheduledDuration);
			}
		}
	}
	if(jobCount != schedule->jobCount) {
		error(1, 0, "Invalid job count %ld in schedule (should be %ld)", schedule->jobCount, jobCount);
	}
}

typedef struct {
	int64_t count;
	int64_t maxItems;
	int64_t* classes;
	double* starts;
	double* ends;
} schedule_timespan_t;

static schedule_timespan_t* create_timespan(int64_t maxItems) {
	schedule_timespan_t* timespan = malloc(sizeof(schedule_timespan_t));
	timespan->count = 0;
	timespan->maxItems = maxItems;
	timespan->classes = calloc(maxItems, sizeof(int64_t));
	timespan->starts = calloc(maxItems, sizeof(double));
	timespan->ends = calloc(maxItems, sizeof(double));
	return timespan;
}

static void delete_timespan(schedule_timespan_t* timespan) {
	free(timespan->classes);
	free(timespan->starts);
	free(timespan->ends);
	free(timespan);
}

static void reset_timespan(schedule_timespan_t* timespan) {
	timespan->count = 0;
}

static void add_timespan(schedule_timespan_t* timespan, int64_t class, double start, double end) {
	if(timespan->count >= timespan->maxItems) {
		error(1, 0, "Tried to add timespan item above limit %ld", timespan->maxItems);
	}
	int64_t i = 0;
	for(; i < timespan->count; i++) {
		if(start >= timespan->ends[i]) continue;
		if(end <= timespan->starts[i]) break;
		error(1, 0, "Conflict in timespan: Found %lf - %lf while adding %lf - %lf", timespan->starts[i], timespan->ends[i], start, end);
	}
	memmove(&timespan->classes[i + 1], &timespan->classes[i], (timespan->count - i) * sizeof(int64_t));
	memmove(&timespan->starts[i + 1], &timespan->starts[i], (timespan->count - i) * sizeof(double));
	memmove(&timespan->ends[i + 1], &timespan->ends[i], (timespan->count - i) * sizeof(double));
	timespan->classes[i] = class;
	timespan->starts[i] = start;
	timespan->ends[i] = end;
	timespan->count++;
}

static void validate_overlaps(instance_t* instance, schedule_t* schedule, bool splittable, bool preemptable) {
	int64_t MAX_ITEMS = 0, MAX_ITEMS_PER_JOB = 0;
	for(int64_t i = 0; i < schedule->jobCount; i++) {
		schedule_job_t* job = &schedule->jobs[i];
		MAX_ITEMS += job->itemCount + 1;
		MAX_ITEMS_PER_JOB = MAX(MAX_ITEMS_PER_JOB, job->itemCount);
	}
	schedule_timespan_t** timespans_by_machine = calloc(instance->machines, sizeof(schedule_timespan_t*));
	for(int64_t i = 0; i < instance->machines; i++) {
		timespans_by_machine[i] = create_timespan(MAX_ITEMS);
	}
	schedule_timespan_t* timespan_by_job = create_timespan(MAX_ITEMS_PER_JOB);

	for(int64_t i = 0; i < schedule->classCount; i++) {
		schedule_class_t* class = &schedule->classes[i];
		for(int64_t j = 0; j < class->jobCount; j++) {
			schedule_job_t* job = &class->jobs[j];
			reset_timespan(timespan_by_job);
			if(!preemptable) {
				if(job->itemCount > 1) {
					error(1, 0, "Job %ld;%ld was splitted", i, j);
				}
			}
			for(int64_t k = 0; k < job->itemCount; k++) {
				schedule_item_t* item = &job->items[k];
				add_timespan(timespans_by_machine[item->machine], i, item->time, item->time + item->duration);
				if(!splittable) add_timespan(timespan_by_job, i, item->time, item->time + item->duration);
				if(item->time + item->duration >= instance->makespan * 3./2. + EPSILON_JUMP) {
					error(1, 0, "Job %ld;%ld with schedule item %ld runs above 3/2 span", i, j, k);
				}
			}
		}
	}

	for(int64_t i = 0; i < instance->machines; i++) {
		int64_t c = -1; double last = 0.;
		schedule_timespan_t* ts = timespans_by_machine[i];
		for(int64_t j = 0; j < ts->count; j++) {
			if(c != ts->classes[j]) {
				c = ts->classes[j];
				if(ts->starts[j] - last + EPSILON_JUMP < instance->classes[c].setup) {
					error(1, 0, "Conflict in timespan: Cannot place setup for class %ld (%lf) at machine %ld between %lf - %lf", c, instance->classes[c].setup, i, last, ts->starts[j]);
				}
			}
			last = ts->ends[j];
		}

		delete_timespan(timespans_by_machine[i]);
	}
	free(timespans_by_machine);
	delete_timespan(timespan_by_job);
}

void validate_schedule(instance_t* instance, schedule_t* schedule, bool splittable, bool preemptable) {
	validate_indices_and_sums(instance, schedule);
	validate_overlaps(instance, schedule, splittable, preemptable);
}
