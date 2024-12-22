#pragma once
#include "types.h"

instance_t* copy_instance(instance_t* instance);
void delete_instance(instance_t* instance);

schedule_t* create_schedule(instance_t* instance);
void reset_schedule(schedule_t* schedule);
void delete_schedule(schedule_t* schedule);

void add_schedule_item(schedule_t* schedule, int64_t machine, double time, double duration, int64_t class, int64_t job);
void remove_last_schedule_item(schedule_t* schedule, int64_t class, int64_t job);
void add_wrap_sequence(schedule_t* schedule, int64_t* machine, double* time, int64_t* wrapclass, double lower, double upper, double duration, double setup, int64_t class, int64_t job);
void add_wrap_position_sequence(schedule_t* schedule, wrap_position_t* wraps, int64_t* index, double* wraptime, int64_t* wrapclass, double duration, double setup, int64_t class, int64_t job);
