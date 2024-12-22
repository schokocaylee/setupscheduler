#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <cjson/cJSON.h>
#define MAX_LEN 0x02000000

static instance_t* read_instance_from_buffer(char* buffer) {
	instance_t* instance = malloc(sizeof(instance_t));
	instance->makespan = -1;
	instance->jobCount = 0;

	cJSON* json = cJSON_Parse(buffer);
	if(json == NULL) {
		error(1, 0, "Could not parse JSON from standard input");
	}

	cJSON* machines = cJSON_GetObjectItemCaseSensitive(json, "machines");
	if(!cJSON_IsNumber(machines)) {
		error(1, 0, "Invalid JSON format for key \"machines\"");
	}
	instance->machines = machines->valueint;

	cJSON* classes = cJSON_GetObjectItemCaseSensitive(json, "classes");
	if(!cJSON_IsArray(classes)) {
		error(1, 0, "Invalid JSON format for key \"classes\"");
	}
	instance->classCount = cJSON_GetArraySize(classes);
	instance->classes = calloc(instance->classCount, sizeof(instance_class_t));
	cJSON* class = classes->child;
	for(size_t i = 0; i < instance->classCount && class != NULL; i++, class = class->next) {
		if(!cJSON_IsObject(class)) {
			error(1, 0, "Invalid JSON format for key \"classes[%zu]\"", i);
		}

		cJSON* setup = cJSON_GetObjectItemCaseSensitive(class, "setup");
		if(!cJSON_IsNumber(setup)) {
			error(1, 0, "Invalid JSON format for key \"classes[%zu].setup\"", i);
		}
		instance->classes[i].setup = setup->valuedouble;

		cJSON* jobs = cJSON_GetObjectItemCaseSensitive(class, "jobs");
		if(!cJSON_IsArray(jobs)) {
			error(1, 0, "Invalid JSON format for key \"classes[%zu].jobs\"", i);
		}
		instance->classes[i].jobCount = cJSON_GetArraySize(jobs);
		instance->jobCount += instance->classes[i].jobCount;
		instance->classes[i].jobs = calloc(instance->classes[i].jobCount, sizeof(instance_job_t));
		cJSON* job = jobs->child;
		for(size_t j = 0; j < instance->classes[i].jobCount && job != NULL; j++, job = job->next) {
			if(!cJSON_IsObject(job)) {
				error(1, 0, "Invalid JSON format for key \"classes[%zu].jobs[%zu]\"", i, j);
			}
			cJSON* duration = cJSON_GetObjectItemCaseSensitive(job, "duration");
			if(!cJSON_IsNumber(duration)) {
				error(1, 0, "Invalid JSON format for key \"classes[%zu].jobs[%zu].duration\"", i, j);
			}
			instance->classes[i].jobs[j].duration = duration->valuedouble;
		}
	}
	cJSON_Delete(json);

	return instance;
}

instance_t* read_instance_from_file(const char* filename) {
	FILE* file = fopen(filename, "r");
	if(file == NULL) {
		error(1, 0, "invalid file name -- '%s'", filename);
	}
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file) + 64;
	rewind(file);
	char* buffer = malloc(sizeof(char) * size);
	fread(buffer, sizeof(char), size, file);
	fclose(file);
	instance_t* instance = read_instance_from_buffer(buffer);
	free(buffer);
	return instance;
}

instance_t* read_instance() {
	char* buffer = malloc(sizeof(char) * MAX_LEN);
	fread(buffer, sizeof(char), MAX_LEN, stdin);
	instance_t* instance = read_instance_from_buffer(buffer);
	free(buffer);
	return instance;
}

static const char* get_category_label(int64_t category) {
	if(CHECK(category, EXPENSIVE_PLUS))  return "E+";
	if(CHECK(category, EXPENSIVE_ZERO))  return "E0";
	if(CHECK(category, EXPENSIVE_MINUS)) return "E-";
	if(CHECK(category, CHEAP_PLUS))      return "C+";
	if(CHECK(category, CHEAP_MINUS))     return "C-";
	return "??";
}

static char* write_instance_to_buffer(const instance_t* instance, const schedule_t* schedule) {
	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "machines", instance->machines);
	if(schedule) {
		cJSON_AddNumberToObject(json, "makespan", instance->makespan);
		cJSON_AddNumberToObject(json, "load", instance->load);
		cJSON_AddNumberToObject(json, "effectiveLoad", instance->effectiveLoad);
		cJSON_AddNumberToObject(json, "longestEffectiveJob", instance->longestEffectiveJob);
		cJSON_AddBoolToObject(json, "preemptKnapsack", CHECK(schedule->category, PREEMPT_KNAPSACK));
	}

	cJSON* jsonClasses = cJSON_AddArrayToObject(json, "classes");
	for(int64_t i = 0; i < instance->classCount; i++) {
		instance_class_t* class = &instance->classes[i];
		cJSON* jsonClass = cJSON_CreateObject();

		cJSON_AddNumberToObject(jsonClass, "setup", class->setup);
		if(schedule) {
			schedule_class_t* classSchedule = &schedule->classes[i];
			cJSON_AddNumberToObject(jsonClass, "load", class->load);
			cJSON_AddNumberToObject(jsonClass, "effectiveLoad", class->effectiveLoad);
			cJSON_AddStringToObject(jsonClass, "type", get_category_label(class->category));
			if(CHECK(class->category, CHEAP_MINUS)) {
				cJSON_AddBoolToObject(jsonClass, "star", CHECK(class->category, STAR));
				if(CHECK(class->category, STAR)) {
					cJSON_AddNumberToObject(jsonClass, "starredLoad", class->starredLoad);
					cJSON_AddBoolToObject(jsonClass, "split", CHECK(class->category, SPLIT));
				}
			}
		}

		cJSON* jsonJobs = cJSON_AddArrayToObject(jsonClass, "jobs");
		for(int64_t j = 0; j < class->jobCount; j++) {
			instance_job_t* job = &class->jobs[j];
			cJSON* jsonJob = cJSON_CreateObject();

			cJSON_AddNumberToObject(jsonJob, "duration", job->duration);
			if(schedule) {
				schedule_class_t* classSchedule = &schedule->classes[i];
				schedule_job_t* jobSchedule = &classSchedule->jobs[j];
				if(CHECK(class->category, CHEAP_MINUS)) {
					cJSON_AddBoolToObject(jsonJob, "star", CHECK(job->category, STAR));
				}

				cJSON* jsonSchedule = cJSON_AddArrayToObject(jsonJob, "schedule");
				for(int64_t k = 0; k < jobSchedule->itemCount; k++) {
					schedule_item_t* item = &jobSchedule->items[k];
					cJSON* jsonItem = cJSON_CreateObject();
					cJSON_AddNumberToObject(jsonItem, "machine", item->machine);
					cJSON_AddNumberToObject(jsonItem, "time", item->time);
					cJSON_AddNumberToObject(jsonItem, "len", item->duration);
					cJSON_AddItemToArray(jsonSchedule, jsonItem);
				}
			}

			cJSON_AddItemToArray(jsonJobs, jsonJob);
		}

		cJSON_AddItemToArray(jsonClasses, jsonClass);
	}

	char* s = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	return s;
}

void write_instance_to_file(const char* filename, const instance_t* instance, const schedule_t* schedule) {
	char* s = write_instance_to_buffer(instance, schedule);
	FILE* file = fopen(filename, "w");
	if(file == NULL) {
		error(1, 0, "invalid file name -- '%s'", filename);
	}
	fwrite(s, sizeof(char), strlen(s), file);
	free(s);
	fclose(file);
}

void write_instance(const instance_t* instance, const schedule_t* schedule) {
	char* s = write_instance_to_buffer(instance, schedule);
	puts(s);
	free(s);
}
