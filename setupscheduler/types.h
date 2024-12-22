#pragma once
#include <stdint.h>
#include <stdbool.h>

#define EPSILON          1e-9
#define EPSILON_JUMP     1e-6

#define CHECK(a, b)      (((a) & (b)) == (b))

#define EXPENSIVE        0x10
#define CHEAP            0x20
#define PLUS             0x01
#define ZERO             0x02
#define MINUS            0x04
#define EXPENSIVE_PLUS   (EXPENSIVE | PLUS)
#define EXPENSIVE_ZERO   (EXPENSIVE | ZERO)
#define EXPENSIVE_MINUS  (EXPENSIVE | MINUS)
#define CHEAP_PLUS       (CHEAP | PLUS)
#define CHEAP_MINUS      (CHEAP | MINUS)
#define STAR             0x100
#define SPLIT            0x200
#define PREEMPT_KNAPSACK 0x1000

typedef struct {
	double duration;

	// Calculated
	int64_t category;
} instance_job_t;

typedef struct {
	double setup;
	int64_t jobCount;
	instance_job_t* jobs;

	// Calculated
	double load;
	double effectiveLoad;
	double longestEffectiveJob;
	int64_t category;
	double starredLoad;
	double splitMode;
} instance_class_t;

typedef struct {
	int64_t classCount;
	int64_t jobCount;
	instance_class_t* classes;
	int64_t machines;

	// Calculated
	double makespan;
	double load;
	double effectiveLoad;
	double longestEffectiveJob;
	double lowerMakespan;
	double upperMakespan;
} instance_t;


typedef struct {
	int64_t machine;
	double time;
	double duration;
} schedule_item_t;

typedef struct {
	int64_t itemCount;
	schedule_item_t* items;

	double scheduledDuration;
} schedule_job_t;

typedef struct {
	int64_t jobCount;
	schedule_job_t* jobs;
} schedule_class_t;

typedef struct {
	int64_t classCount;
	int64_t jobCount;
	schedule_class_t* classes;
	schedule_job_t* jobs;

	int64_t category;
} schedule_t;


typedef bool (*scheduler_t)(instance_t*, schedule_t*);
typedef bool (*scheduletester_t)(instance_t*);


typedef struct {
	int64_t machine;
	double lower;
	double upper;
} wrap_position_t;
