#include "test.h"
#include "prepare.h"
#include "basic.h"

bool testBinary(double makespan, instance_t* instance, scheduletester_t scheduler) {
	if(makespan < instance->lowerMakespan) return false;
	if(makespan >= instance->upperMakespan) return true;
	prepare_instance(instance, makespan);
	return scheduler(instance);
}
