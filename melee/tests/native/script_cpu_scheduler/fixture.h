#ifndef SCRIPT_CPU_SCHEDULER_FIXTURE_H
#define SCRIPT_CPU_SCHEDULER_FIXTURE_H
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage cpu_package;
extern unsigned mutation_mode,write_count,mode_tails,output_tails,queue_count;
extern unsigned tick_order;
extern HSD_GObj* mutation_actor,*replacement;
extern HSD_GObj* (*reuse_actor)(void);
extern float random_sample;
extern bool item_attack;
void cpu_mutate(HSD_GObj*);
#endif
