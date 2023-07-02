#pragma once

#include <stdint.h>
#include "types.h"
#include "burrow.h"

#define SQUIRM_MEM_SIZE 0x10000

typedef void (*squirm_mmio_write_fn)(u16 addr, u8 val);
typedef u8 (*squirm_mmio_read_fn)(u16 addr);

typedef struct squirm_cpu squirm_cpu_t;

typedef void (*squirm_cpu_syscall_fn)(squirm_cpu_t* cpu);

typedef struct squirm_mmio_entry {
    u16 start;
    u16 end;
    squirm_mmio_write_fn write;
    squirm_mmio_read_fn read;
} squirm_mmio_entry_t;

#define SQUIRM_MMIO_MAX 16

typedef struct squirm_cpu {
    u16 reg[BURROW_REG_COUNT];
    u8 mem[SQUIRM_MEM_SIZE];
    usize executed_op_count;

    squirm_mmio_entry_t mmio[SQUIRM_MMIO_MAX];
    u16 mmio_count;

    squirm_cpu_syscall_fn syscalls[BURROW_SYS_MAX_SYSCALLS];
    u16 syscall_count;
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

#define OP_HANDLER_NAME(OP) squirm_cpu_op_##OP
#define OP_HANDLER(OP) static void OP_HANDLER_NAME(OP)(squirm_cpu_t * cpu, squirm_op_t op)

squirm_cpu_t* squirm_cpu_new(squirm_cpu_syscall_fn* syscalls, u16 num_sys);
void squirm_cpu_free(squirm_cpu_t* cpu);
void squirm_cpu_add_mmio_entry(squirm_cpu_t* cpu, squirm_mmio_entry_t entry);
void squirm_cpu_reset(squirm_cpu_t* cpu);
void squirm_cpu_load(squirm_cpu_t* cpu, u8* data, u16 size);
void squirm_cpu_load_at(squirm_cpu_t* cpu, u16 addr, u8* data, u16 size);
void squirm_cpu_step(squirm_cpu_t* cpu);
