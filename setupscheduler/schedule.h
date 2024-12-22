#pragma once
#include "json.h"
#include "basic.h"
#include <stdint.h>
#include <stdbool.h>

bool test_schedule_preempt(instance_t* instance);
bool schedule_preempt_lpmtn(instance_t* instance, double* lpmtn);
bool schedule_preempt(instance_t* instance, schedule_t* schedule);
int64_t schedule_expensive_plus_get_machine_count(instance_t* instance, instance_class_t* class);

bool test_schedule_split(instance_t* instance);
bool schedule_split_lsplit(instance_t* instance, double* lsplit);
bool schedule_split(instance_t* instance, schedule_t* schedule);
int64_t schedule_expensive_get_machine_count(instance_t* instance, instance_class_t* class);

bool test_schedule_nonpreempt(instance_t* instance);
bool schedule_nonpreempt_lnonp(instance_t* instance, double* lnonp);
bool schedule_nonpreempt(instance_t* instance, schedule_t* schedule);
