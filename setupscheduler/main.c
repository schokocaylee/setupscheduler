#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include "ctimer.h"
#include "json.h"
#include "basic.h"
#include "prepare.h"
#include "schedule.h"
#include "search.h"
#include "generate.h"
#include "validate.h"
#include "jump.h"

static void init_random() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	srand(ts.tv_nsec);
}

static instance_t* instance = NULL;
static schedule_t* schedule = NULL;
static bool quiet = false;

static enum { INPUT_STDIN, INPUT_FILENAME, INPUT_GENERATE } inputMode = INPUT_STDIN;
static char* inputFilename;
static int64_t inputGenerateFactor;
static int64_t inputGenerateFactorMachine;
static int64_t inputGenerateFactorClass;
static int64_t inputGenerateFactorMakespan;
static void do_input() {
	ctimer_t timer;
	ctimer_start(&timer);

	switch(inputMode) {
		case INPUT_STDIN:
			instance = read_instance();
			break;
		case INPUT_FILENAME:
			instance = read_instance_from_file(inputFilename);
			break;
		case INPUT_GENERATE:
			instance = generate_instance(inputGenerateFactorMachine, inputGenerateFactorClass, inputGenerateFactorMakespan);
			break;
	}

	ctimer_stop(&timer);
	ctimer_measure(&timer);
	if(!quiet) {
		ctimer_print(timer, "input");
	}
}

static enum { ANALYZE_NONE, ANALYZE_ANALYZE } analyzeMode = ANALYZE_NONE;
static enum { CALC_NONE, CALC_MAKESPAN, CALC_BINARY, CALC_JUMP, CALC_SAFE } calcMode = CALC_JUMP;
static enum { VARIANT_PREEMPT, VARIANT_SPLIT, VARIANT_NONPREEMPT } variantMode = VARIANT_PREEMPT;
static double calcMakespan = -1;

static void do_analyze() {
	if(analyzeMode == ANALYZE_NONE) return;

	ctimer_t timer;
	ctimer_start(&timer);

	prepare_instance(instance, 1);
	fprintf(stderr, "\n------------------------- ANALYSIS -------------------------\n\n");
	fprintf(stderr, "Machine count:             %8ld\n", instance->machines);
	fprintf(stderr, "Class count:               %8ld\n", instance->classCount);
	fprintf(stderr, "Job count:                 %8ld\n", instance->jobCount);
	fprintf(stderr, "Instance load:             %11.2lf\n", instance->load);
	fprintf(stderr, "Instance effective load:   %11.2lf\n", instance->effectiveLoad);
	fprintf(stderr, "Longest effective job:     %11.2lf\n", instance->longestEffectiveJob);
	fprintf(stderr, "Makespan range:            %11.2lf  -  %11.2lf\n", instance->lowerMakespan, instance->upperMakespan);
	fprintf(stderr, "\n------------------------------------------------------------\n\n");

	if(calcMakespan > 0) {
		prepare_instance(instance, calcMakespan);
		fprintf(stderr, "\n------------------ MAKESPAN-BASED ANALYSIS -----------------\n\n");
		fprintf(stderr, "Makespan:                  %11.2lf\n", instance->makespan);
		int64_t eplus = 0, ezero = 0, eminus = 0, cplus = 0, cminus = 0, star = 0;
		for(int64_t i = 0; i < instance->classCount; i++) {
			instance_class_t* class = &instance->classes[i];
			if(CHECK(class->category, EXPENSIVE_PLUS)) {
				eplus++;
			} else if(CHECK(class->category, EXPENSIVE_ZERO)) {
				ezero++;
			} else if(CHECK(class->category, EXPENSIVE_MINUS)) {
				eminus++;
			} else if(CHECK(class->category, CHEAP_PLUS)) {
				cplus++;
			} else if(CHECK(class->category, CHEAP_MINUS)) {
				cminus++;
				if(CHECK(class->category, STAR)) {
					star++;
				}
			}
		}
		fprintf(stderr, "E  classes:                %8ld\n", eplus + ezero + eminus);
		fprintf(stderr, " + classes:                %8ld\n", eplus);
		fprintf(stderr, " 0 classes:                %8ld\n", ezero);
		fprintf(stderr, " - classes:                %8ld\n", eminus);
		fprintf(stderr, "C  classes:                %8ld\n", cplus + cminus);
		fprintf(stderr, " + classes:                %8ld\n", cplus);
		fprintf(stderr, " - classes:                %8ld (with %8ld in C*)\n", cminus, star);
		fprintf(stderr, "\n------------------------------------------------------------\n\n");
	}

	ctimer_stop(&timer);
	ctimer_measure(&timer);
	if(!quiet) {
		ctimer_print(timer, "analyze");
	}
}

static void do_calc() {
	if(calcMode == CALC_NONE) return;
	if(variantMode == VARIANT_NONPREEMPT && (calcMode == CALC_JUMP || calcMode == CALC_SAFE)) {
		calcMode = CALC_BINARY;
	}

	ctimer_t timer;
	ctimer_start(&timer);

	if(calcMode == CALC_BINARY || calcMode == CALC_SAFE) {
		scheduletester_t tester = NULL;
		switch(variantMode) {
			case VARIANT_PREEMPT:
				tester = test_schedule_preempt;
				break;
			case VARIANT_SPLIT:
				tester = test_schedule_split;
				break;
			case VARIANT_NONPREEMPT:
				tester = test_schedule_nonpreempt;
				break;
		}
		calcMakespan = scheduler_binarysearch(instance, tester);
		if(calcMode == CALC_SAFE) {
			fprintf(stderr, "Binary makespan: %.6lf\n", calcMakespan);
		}
	}
	if(calcMode == CALC_JUMP || calcMode == CALC_SAFE) {
		switch(variantMode) {
			case VARIANT_PREEMPT:
				calcMakespan = preempt_classjump(instance);
				break;
			case VARIANT_SPLIT:
				calcMakespan = split_classjump(instance);
				break;
			case VARIANT_NONPREEMPT:
				/* cannot happen */
				break;
		}
	}
	if(calcMode != CALC_MAKESPAN) {
		if(calcMakespan < instance->lowerMakespan) {
			calcMakespan = instance->lowerMakespan;
		}
	}

	scheduler_t scheduler = NULL;
	switch(variantMode) {
		case VARIANT_PREEMPT:
			scheduler = schedule_preempt;
			break;
		case VARIANT_SPLIT:
			scheduler = schedule_split;
			break;
		case VARIANT_NONPREEMPT:
			scheduler = schedule_nonpreempt;
			break;
	}
	schedule = scheduler_makespan(instance, scheduler, calcMakespan);
	if(schedule == NULL) {
		error(1, 0, "no valid solution for makespan %.6lf", calcMakespan);
	}

	ctimer_stop(&timer);
	ctimer_measure(&timer);
	if(!quiet) {
		ctimer_print(timer, "calculate");
		fprintf(stderr, "Found solution with makespan %.6lf\n", instance->makespan);
	}
}

static enum { VALIDATE_NONE, VALIDATE_VALIDATE } validateMode = VALIDATE_NONE;
static void do_validate() {
	if(validateMode == VALIDATE_NONE) return;

	ctimer_t timer;
	ctimer_start(&timer);

	validate_schedule(instance, schedule, variantMode == VARIANT_SPLIT, variantMode != VARIANT_NONPREEMPT);

	ctimer_stop(&timer);
	ctimer_measure(&timer);
	if(!quiet) {
		ctimer_print(timer, "validate");
	}
}

static enum { PRINT_NONE, PRINT_STDOUT, PRINT_FILENAME } printMode = PRINT_STDOUT;
static enum { FORMAT_SCHEDULE_JSON, FORMAT_INSTANCE_JSON } formatMode = FORMAT_SCHEDULE_JSON;
static char* printFilename;
static void do_print() {
	if(printMode == PRINT_NONE) return;

	ctimer_t timer;
	ctimer_start(&timer);

	schedule_t* printSchedule = NULL;
	if(formatMode == FORMAT_SCHEDULE_JSON) {
		printSchedule = schedule;
	}

	switch(printMode) {
		case PRINT_STDOUT:
			write_instance(instance, printSchedule);
			break;
		case PRINT_FILENAME:
			write_instance_to_file(printFilename, instance, printSchedule);
			break;
		default:
			break;
	}

	ctimer_stop(&timer);
	ctimer_measure(&timer);
	if(!quiet) {
		ctimer_print(timer, "print");
	}
}

static void show_usage();
static void show_version();

int main(int argc, char** argv) {
	while(1) {
		static struct option opts[] = {
			{ "help",          no_argument,       0, 'h' },
			{ "version",       no_argument,       0, 'v' },
			{ "quiet",         no_argument,       0, 'q' },
			{ "instance",      required_argument, 0, 'i' },
			{ "generate",      required_argument, 0, 'g' },
			{ "factor-machine",   required_argument, 0, 'M' },
			{ "factor-class",     required_argument, 0, 'c' },
			{ "factor-makespan",  required_argument, 0, 'k' },
			{ "makespan",      required_argument, 0, 'm' },
			{ "preempt",       no_argument,       0, 'p' },
			{ "split",         no_argument,       0, 's' },
			{ "nonpreempt",    no_argument,       0, 'n' },
			{ "analyze",       no_argument,       0, 'a' },
			{ "skip-calc",     no_argument,       0, 'C' },
			{ "skip-schedule", no_argument,       0, 'S' },
			{ "binary",        no_argument,       0, 'b' },
			{ "safe",          no_argument,       0, 'B' },
			{ "validate",      no_argument,       0, 'V' },
			{ "output",        required_argument, 0, 'o' },
			{ "no-output",     no_argument,       0, 'Q' },
		};
		int c = getopt_long(argc, argv, "hvqi:g:m:psnaCSbBVo:Q", opts, NULL);
		if(c == -1) break;
		switch(c) {
			case 'h':
				show_usage();
				exit(0);
			case 'v':
				show_version();
				exit(0);
			case 'q':
				quiet = true;
				break;
			case 'i':
				inputMode = INPUT_FILENAME;
				inputFilename = optarg;
				break;
			case 'g':
				inputMode = INPUT_GENERATE;
				inputGenerateFactor = strtoll(optarg, NULL, 10);
				if(inputGenerateFactor <= 0) {
					error(1, 0, "invalid generation factor -- '%s'", optarg);
				}
				inputGenerateFactorMachine = 14 * inputGenerateFactor;
				inputGenerateFactorClass = 16 * inputGenerateFactor;
				inputGenerateFactorMakespan = 1000 * sqrt(inputGenerateFactor);
				break;
			case 'M':
				inputGenerateFactorMachine = strtoll(optarg, NULL, 10);
				if(inputGenerateFactorMachine <= 0) {
					error(1, 0, "invalid machine generation factor -- '%s'", optarg);
				}
				break;
			case 'c':
				inputGenerateFactorClass = strtoll(optarg, NULL, 10);
				if(inputGenerateFactorClass <= 0) {
					error(1, 0, "invalid class generation factor -- '%s'", optarg);
				}
				break;
			case 'k':
				inputGenerateFactorMakespan = strtoll(optarg, NULL, 10);
				if(inputGenerateFactorMakespan <= 0) {
					error(1, 0, "invalid makespan generation factor -- '%s'", optarg);
				}
				break;
			case 'm':
				calcMode = CALC_MAKESPAN;
				calcMakespan = strtod(optarg, NULL);
				if(calcMakespan <= 0) {
					error(1, 0, "invalid makespan -- '%s'", optarg);
				}
				break;
			case 'p':
				variantMode = VARIANT_PREEMPT;
				break;
			case 's':
				variantMode = VARIANT_SPLIT;
				break;
			case 'n':
				variantMode = VARIANT_NONPREEMPT;
				break;
			case 'a':
				analyzeMode = ANALYZE_ANALYZE;
				break;
			case 'C':
				calcMode = CALC_NONE;
			case 'S':
				formatMode = FORMAT_INSTANCE_JSON;
				break;
			case 'b':
				calcMode = CALC_BINARY;
				break;
			case 'B':
				calcMode = CALC_SAFE;
				break;
			case 'V':
				validateMode = VALIDATE_VALIDATE;
				break;
			case 'o':
				printMode = PRINT_FILENAME;
				printFilename = optarg;
				break;
			case 'Q':
				printMode = PRINT_NONE;
				break;
			default:
				exit(1);
		}
	}
	if(optind < argc) {
		error(1, 0, "invalid extra argument -- '%s'", argv[optind]);
	}

	init_random();
	do_input();
	do_analyze();
	do_calc();
	do_validate();
	do_print();

	if(schedule) {
		delete_schedule(schedule);
	}
	delete_instance(instance);
	return 0;
}

static void show_usage() {
	printf("Usage: %s [OPTION]...\n", program_invocation_name);
	puts("Calculate a 3/2-scheduling for problems with batch setup times");
	puts("");
	puts("  -h, --help                    display this help and exit");
	puts("  -v, --version                 display version information and exit");
	puts("  -q, --quiet                   do not show runtime stats");
	puts("");
	puts("  -i, --instance[=FILENAME]     read instance from FILENAME using standard JSON format");
	puts("  -g, --generate[=FACTOR]       generate a problem instance, using FACTOR as size orientation");
	puts("  -M, --factor-machine[=MACHINEFACTOR]   use MACHINEFACTOR as machine count for generator (default: 14 * FACTOR)");
	puts("  -c, --factor-class[=CLASSFACTOR]       use CLASSFACTOR as class count for generator (default: 16 * FACTOR)");
	puts("  -k, --factor-makespan[=MAKESPAN]       use MAKESPAN as makespan for generator (default: 1000 * sqrt(FACTOR))");
	puts("  -m, --makespan[=MAKESPAN]     use fixed makespan MAKESPAN and fail if impossible");
	puts("  -p, --preempt                 use preemptive scheduling (default)");
	puts("  -s, --split                   use splittable scheduling");
	puts("  -n, --nonpreempt              use non-preemptive scheduling");
	puts("  -a, --analyze                 show basic instance information before calculation");
	puts("  -C, --skip-calc               just generate instance and print it, do not calculate schedule");
	puts("  -b, --binary                  use binary makespan search instead of class jumping");
	puts("  -B, --safe                    use class jumping, but calculate and print binary makespan search result too");
	puts("  -V, --validate                validate the schedule for correctness and makespan constraints");
	puts("  -o, --output[=FILENAME]       write schedule to FILENAME using standard JSON format");
	puts("  -Q, --no-output               just calculate schedule and do not print it (dry-run)");
	puts("  -S, --skip-schedule           calculate schedule, but print instance only");
	puts("");
	puts("Report bugs to: Hendrik Oenings <hendrik.oenings@stu.uni-kiel.de>");
}
static void show_version() {
	puts("setupscheduler");
	puts("");
	puts("Copyright (C) 2024 Hendrik Oenings");
	puts(
		"License AGPLv3+: GNU AGPL version 3 or later <http://gnu.org/licenses/agpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law."
	);
}
