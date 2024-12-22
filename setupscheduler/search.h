#pragma once
#include "types.h"

int64_t scheduler_binarysearch(instance_t* instance, scheduletester_t scheduler);
schedule_t* scheduler_makespan(instance_t* instance, scheduler_t scheduler, double makespan);
