#pragma once

#include <stdint.h>
#include "types.h"
#include "burrow.h"

#define SQUIRM_MEM_SIZE 0x10000

typedef struct squirm_cpu {
    u16 reg[BURROW_REG_COUNT];
    u8 mem[SQUIRM_MEM_SIZE];
} squirm_cpu_t;

typedef struct squirm_op {
    u8 op;
    struct {
        u8 dest;
        u8 src_a;
        u8 src_b;
    } args;
} squirm_op_t;

typedef void (*squirm_cpu_op_fn)(squirm_cpu_t* cpu, squirm_op_t op);
typedef void (*squirm_cpu_syscall_fn)(squirm_cpu_t* cpu);

#define OP_HANDLER_NAME(OP) squirm_cpu_op_##OP
#define OP_HANDLER(OP) static void OP_HANDLER_NAME(OP)(squirm_cpu_t * cpu, squirm_op_t op)

void squirm_cpu_init(squirm_cpu_t* cpu, squirm_cpu_syscall_fn* syscalls, u16 num_sys);
void squirm_cpu_free(squirm_cpu_t* cpu);
void squirm_cpu_reset(squirm_cpu_t* cpu);
void squirm_cpu_load(squirm_cpu_t* cpu, u8* data, u16 size);
void squirm_cpu_load_at(squirm_cpu_t* cpu, u16 addr, u8* data, u16 size);
void squirm_cpu_step(squirm_cpu_t* cpu);
