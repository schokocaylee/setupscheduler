#pragma once
#include "types.h"
#include <stdint.h>

instance_t* read_instance();
instance_t* read_instance_from_file(const char* filename);
void write_instance(const instance_t* instance, const schedule_t* schedule);
void write_instance_to_file(const char* filename, const instance_t* instance, const schedule_t* schedule);
