#include "basic.h"
#include <stdlib.h>
#include <string.h>

instance_t* copy_instance(instance_t* data) {
	instance_t* instance = malloc(sizeof(instance_t));
	instance->classCount = data->classCount;
	instance->jobCount = data->jobCount;
	instance->machines = data->machines;
	instance->classes = calloc(instance->classCount, sizeof(instance_class_t));
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		instance_class_t* dataClass = &data->classes[i];
		class->setup = dataClass->setup;
		class->jobCount = dataClass->jobCount;
		class->jobs = malloc(class->jobCount * sizeof(instance_job_t));
		memcpy(class->jobs, dataClass->jobs, class->jobCount * sizeof(instance_job_t));
	}
	return instance;
}
void delete_instance(instance_t* instance) {
	for(int64_t i = 0; i < instance->classCount; i++) {
		free(instance->classes[i].jobs);
	}
	free(instance->classes);
	free(instance);
}

schedule_t* create_schedule(instance_t* instance) {
	schedule_t* schedule = malloc(sizeof(schedule_t));
	schedule->category = 0;
	schedule->classCount = instance->classCount;
	schedule->jobCount = instance->jobCount;
	schedule->classes = calloc(schedule->classCount, sizeof(schedule_class_t));
	schedule->jobs = calloc(schedule->jobCount, sizeof(schedule_job_t));
	for(int64_t i = 0, j = 0; i < schedule->classCount; i++) {
		schedule_class_t* class = &schedule->classes[i];
		instance_class_t* instanceClass = &instance->classes[i];
		class->jobCount = instanceClass->jobCount;
		class->jobs = &schedule->jobs[j];
		j += class->jobCount;
	}
	return schedule;
}

void reset_schedule(schedule_t* schedule) {
	schedule->category = 0;
	for(int64_t i = 0; i < schedule->jobCount; i++) {
		schedule_job_t* job = &schedule->jobs[i];
		free(job->items);
	}
	memset(schedule->jobs, 0, schedule->jobCount * sizeof(schedule_job_t));
}

void delete_schedule(schedule_t* schedule) {
	for(int64_t i = 0; i < schedule->jobCount; i++) {
		schedule_job_t* job = &schedule->jobs[i];
		free(job->items);
	}
	free(schedule->jobs);
	free(schedule->classes);
	free(schedule);
}

void add_schedule_item(schedule_t* schedule, int64_t machine, double time, double duration, int64_t class, int64_t job) {
	schedule_class_t* classSchedule = &schedule->classes[class];
	schedule_job_t* jobSchedule = &classSchedule->jobs[job];
	jobSchedule->items = reallocarray(jobSchedule->items, jobSchedule->itemCount + 1, sizeof(schedule_item_t));
	schedule_item_t* item = &jobSchedule->items[jobSchedule->itemCount];
	item->machine = machine;
	item->time = time;
	item->duration = duration;
	jobSchedule->itemCount++;
	jobSchedule->scheduledDuration += duration;
}

void remove_last_schedule_item(schedule_t* schedule, int64_t class, int64_t job) {
	schedule_class_t* classSchedule = &schedule->classes[class];
	schedule_job_t* jobSchedule = &classSchedule->jobs[job];
	schedule_item_t* item = &jobSchedule->items[--jobSchedule->itemCount];
	jobSchedule->scheduledDuration -= item->duration;
}

void add_wrap_sequence(schedule_t* schedule, int64_t* machine, double* time, int64_t* wrapclass, double lower, double upper, double duration, double setup, int64_t class, int64_t job) {
	if(duration <= EPSILON) return;
	if(*wrapclass != class) {
		*time += setup;
		*wrapclass = class;
	}
	double remain = upper - *time;
	if(remain <= EPSILON) {
		(*machine)++;
		*time = lower;
		remain = upper - lower;
	}
	if(duration > remain + EPSILON) {
		add_schedule_item(schedule, *machine, *time, remain, class, job);
		(*machine)++; *time = lower;
		add_schedule_item(schedule, *machine, *time, duration - remain, class, job);
		*time += duration - remain;
	} else {
		add_schedule_item(schedule, *machine, *time, duration, class, job);
		*time += duration;
	}
}

void add_wrap_position_sequence(schedule_t* schedule, wrap_position_t* wraps, int64_t* index, double* wraptime, int64_t* wrapclass, double duration, double setup, int64_t class, int64_t job) {
       if(duration <= EPSILON) return;
       if(*wrapclass != class) {
               *wraptime += setup;
               *wrapclass = class;
       }
       double remain = wraps[*index].upper - wraps[*index].lower - *wraptime;
       if(remain <= EPSILON) {
               (*index)++; *wraptime = 0;
               remain = wraps[*index].upper - wraps[*index].lower;
       }
       while(duration > EPSILON) {
               if(duration <= remain + EPSILON) {
                       add_schedule_item(schedule, wraps[*index].machine, wraps[*index].lower + *wraptime, duration, class, job);
                       *wraptime += duration;
                       break;
               }
               add_schedule_item(schedule, wraps[*index].machine, wraps[*index].lower + *wraptime, remain, class, job);
               duration -= remain;
               (*index)++; *wraptime = 0;
               remain = wraps[*index].upper - wraps[*index].lower;
       }
}
