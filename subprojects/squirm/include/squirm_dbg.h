#pragma once

#include "burrow.h"
#include "squirm.h"
#include "types.h"
#include <stdio.h>

typedef struct squirm_dbg_breakpoint {
    u16 addr;
} squirm_dbg_breakpoint_t;

typedef struct squirm_dbg_watchpoint {
    u16 addr;
} squirm_dbg_watchpoint_t;

typedef struct squirm_dbg {
    squirm_cpu_t* cpu;

    // debugging
    squirm_dbg_breakpoint_t breakpoints[256];
    squirm_dbg_watchpoint_t watchpoints[256];
    u8 breakpoint_count;
    u8 watchpoint_count;

    bool regwatch[BURROW_REG_COUNT];

    // state
    u16 prev_regs[BURROW_REG_COUNT];
    bool running;
} squirm_dbg_t;

squirm_dbg_t* squirm_dbg_new(squirm_cpu_t* cpu);
void squirm_dbg_free(squirm_dbg_t* dbg);

void squirm_dbg_step(squirm_dbg_t* dbg);
void squirm_dbg_run(squirm_dbg_t* dbg);
