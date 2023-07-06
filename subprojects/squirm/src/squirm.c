#include "squirm.h"
#include "burrow.h"
#include "types.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

squirm_cpu_t* squirm_cpu_new(squirm_cpu_syscall_fn* syscalls, u16 num_sys) {
    squirm_cpu_t* cpu = malloc(sizeof(squirm_cpu_t));

    for (int i = 0; i < BURROW_REG_COUNT; i++) {
        cpu->reg[i] = 0;
    }

    for (int i = 0; i < BURROW_MEM_SIZE; i++) {
        cpu->mem[i] = 0;
    }

    if (num_sys > BURROW_SYS_MAX_SYSCALLS) {
        LOG_ERROR("Too many syscalls: %d\n", num_sys);
        exit(1);
    }

    for (int i = 0; i < num_sys; i++) {
        cpu->syscalls[i] = syscalls[i];
    }

    cpu->syscall_count = num_sys;

    cpu->mmio_count = 0;

    return cpu;
}

void squirm_cpu_add_mmio_entry(squirm_cpu_t* cpu, squirm_mmio_entry_t entry) {
    if (cpu->mmio_count >= SQUIRM_MMIO_MAX) {
        LOG_ERROR("too many MMIO entries: %d\n", cpu->mmio_count);
        exit(1);
    }

    cpu->mmio[cpu->mmio_count++] = entry;
    LOG_DEBUG("added MMIO entry from %04X to %04X\n", entry.start, entry.end);
}

void squirm_cpu_free(squirm_cpu_t* cpu) {
    free(cpu);
}

void squirm_cpu_reset(squirm_cpu_t* cpu) {
    for (int i = 0; i < BURROW_REG_COUNT; i++) {
        cpu->reg[i] = 0;
    }

    cpu->reg[BURROW_REG_IP] = BURROW_MEM_CODE_START;
    cpu->reg[BURROW_REG_SP] = BURROW_MEM_STACK_START;
    cpu->reg[BURROW_REG_FL] = BURROW_FL_NONE;

    cpu->executed_op_count = 0;
}

void squirm_cpu_load(squirm_cpu_t* cpu, u8* data, u16 size) {
    memcpy(cpu->mem, data, size);
}

void squirm_cpu_load_at(squirm_cpu_t* cpu, u16 addr, u8* data, u16 size) {
    memcpy(cpu->mem + addr, data, size);
}

static inline u8 squirm_cpu_read8(squirm_cpu_t* cpu, u16 addr) {
    return cpu->mem[addr];
}

static inline u16 squirm_cpu_read16(squirm_cpu_t* cpu, u16 addr) {
    return cpu->mem[addr] | (cpu->mem[addr + 1] << 8);
}

static inline void squirm_cpu_write8(squirm_cpu_t* cpu, u16 addr, u8 value) {
    cpu->mem[addr] = value;
}

static inline void squirm_cpu_write16(squirm_cpu_t* cpu, u16 addr, u16 value) {
    cpu->mem[addr] = value & 0xFF;
    cpu->mem[addr + 1] = (value >> 8) & 0xFF;
}

static inline u16 squirm_cpu_read_reg(squirm_cpu_t* cpu, u8 reg) {
    return cpu->reg[reg];
}

static inline void squirm_cpu_write_reg(squirm_cpu_t* cpu, u8 reg, u16 value) {
    cpu->reg[reg] = value;
}

static inline u16 squirm_cpu_flags(squirm_cpu_t* cpu) {
    return cpu->reg[BURROW_REG_FL];
}

static inline void squirm_cpu_set_flags(squirm_cpu_t* cpu, u16 flags) {
    cpu->reg[BURROW_REG_FL] = flags;
}

static inline void squirm_cpu_set_flag(squirm_cpu_t* cpu, u16 flag) {
    squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) | flag);
}

static inline u16 squirm_op_imm(squirm_op_t op) {
    return (op.args.src_a << 8) | (op.args.src_b & 0xFF);
}

OP_HANDLER(nop) {
    (void)cpu; // unused
    (void)op;  // unused
    // nothing to do
}

OP_HANDLER(ldi) {
    u8 reg = op.args.dest;
    u16 value = squirm_op_imm(op);
    squirm_cpu_write_reg(cpu, reg, value);
}

OP_HANDLER(ldr) {
    u8 dest = op.args.dest;
    u16 addr = squirm_op_imm(op);

    for (int i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.read == NULL) {
                return;
            }
            u8 value_hi = entry.read(addr);
            u8 value_lo = entry.read(addr + 1);
            u16 value = value_hi | (value_lo << 8);
            squirm_cpu_write_reg(cpu, dest, value);
            return;
        }
    }

    u16 value = squirm_cpu_read16(cpu, addr);
    squirm_cpu_write_reg(cpu, dest, value);
}

OP_HANDLER(ldrb) {
    u8 dest = op.args.dest;
    u16 addr = squirm_op_imm(op);
    for (int i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.read == NULL) {
                return;
            }
            u8 value = entry.read(addr);
            squirm_cpu_write_reg(cpu, dest, value);
            return;
        }
    }
    u8 value = squirm_cpu_read8(cpu, addr);
    squirm_cpu_write_reg(cpu, dest, value);
}

OP_HANDLER(add) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) + squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }
    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(sub) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) - squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }
    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(mul) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) * squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(div) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;

    u16 b_value = squirm_cpu_read_reg(cpu, b);

    if (b_value == 0) {
        LOG_ERROR("division by zero at %04x\n", cpu->reg[BURROW_REG_IP] - 4);
        exit(1);
    }

    u16 result = squirm_cpu_read_reg(cpu, a) / b_value;

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(mod) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 b_value = squirm_cpu_read_reg(cpu, b);

    if (b_value == 0) {
        LOG_ERROR("division by zero at %04x\n", cpu->reg[BURROW_REG_IP] - 4);
        exit(1);
    }

    u16 result = squirm_cpu_read_reg(cpu, a) % b_value;

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(and) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) & squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(or) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) | squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(xor) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u8 b = op.args.src_b;
    u16 result = squirm_cpu_read_reg(cpu, a) ^ squirm_cpu_read_reg(cpu, b);

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(shl) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u16 b = squirm_cpu_read_reg(cpu, op.args.src_b);
    u16 result = squirm_cpu_read_reg(cpu, a) << b;

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(shr) {
    u8 dest = op.args.dest;
    u8 a = op.args.src_a;
    u16 b = squirm_cpu_read_reg(cpu, op.args.src_b);
    u16 result = squirm_cpu_read_reg(cpu, a) >> b;

    if (result == 0) {
        squirm_cpu_set_flag(cpu, BURROW_FL_ZERO);
    } else {
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }

    squirm_cpu_write_reg(cpu, dest, result);
}

OP_HANDLER(jmp) {
    u16 addr = squirm_op_imm(op);

    cpu->reg[BURROW_REG_IP] = addr;
}

OP_HANDLER(jz) {
    u16 addr = squirm_op_imm(op);
    if (squirm_cpu_flags(cpu) & BURROW_FL_ZERO) {
        cpu->reg[BURROW_REG_IP] = addr;
        squirm_cpu_set_flags(cpu, squirm_cpu_flags(cpu) & ~BURROW_FL_ZERO);
    }
}

OP_HANDLER(jd) {
    u16 addr = squirm_cpu_read_reg(cpu, op.args.dest);
    cpu->reg[BURROW_REG_IP] = addr;
}

OP_HANDLER(sti) {
    u16 addr = squirm_op_imm(op);
    u16 value = squirm_cpu_read_reg(cpu, op.args.dest);

    for (u16 i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.write == NULL) {
                return;
            }
            u8 value_hi = (value >> 8) & 0xff;
            u8 value_lo = value & 0xff;
            entry.write(addr, value_hi);
            entry.write(addr + 1, value_lo);
            return;
        }
    }

    squirm_cpu_write16(cpu, addr, value);
}

OP_HANDLER(stib) {
    u16 addr = squirm_op_imm(op);
    u8 value = squirm_cpu_read_reg(cpu, op.args.dest);

    for (u16 i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.write == NULL) {
                return;
            }
            cpu->mmio[i].write(addr, value);
            return;
        }
    }

    squirm_cpu_write8(cpu, addr, value);
}

OP_HANDLER(str) {
    u16 addr = squirm_cpu_read_reg(cpu, op.args.dest);
    u16 value = squirm_cpu_read_reg(cpu, op.args.src_a);

    for (u16 i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.write == NULL) {
                return;
            }
            u8 value_hi = (value >> 8) & 0xff;
            u8 value_lo = value & 0xff;
            entry.write(addr, value_hi);
            entry.write(addr + 1, value_lo);
            return;
        }
    }

    squirm_cpu_write16(cpu, addr, value);
}

OP_HANDLER(strb) {
    u16 addr = squirm_cpu_read_reg(cpu, op.args.dest);
    u8 value = squirm_cpu_read_reg(cpu, op.args.src_a);

    for (u16 i = 0; i < cpu->mmio_count; i++) {
        squirm_mmio_entry_t entry = cpu->mmio[i];
        if (entry.start <= addr && addr < entry.end) {
            if (entry.write == NULL) {
                return;
            }
            entry.write(addr, value);
            return;
        }
    }

    squirm_cpu_write8(cpu, addr, value);
}

OP_HANDLER(sys) {
    (void)op; // unused
    u16 syscall = cpu->reg[BURROW_REG_A];

    if (syscall >= cpu->syscall_count) {
        LOG_ERROR("invalid syscall %d\n", syscall);
        exit(1);
    }

    cpu->syscalls[syscall](cpu);
}

// clang-format off

static const squirm_cpu_op_fn k_op_handlers[BURROW_OP_COUNT] = {
    OP_HANDLER_NAME(nop),
    OP_HANDLER_NAME(ldi),
    OP_HANDLER_NAME(ldr),
    OP_HANDLER_NAME(ldrb),
    OP_HANDLER_NAME(add),
    OP_HANDLER_NAME(sub),
    OP_HANDLER_NAME(mul),
    OP_HANDLER_NAME(div),
    OP_HANDLER_NAME(mod),
    OP_HANDLER_NAME(and),
    OP_HANDLER_NAME(or),
    OP_HANDLER_NAME(xor),
    OP_HANDLER_NAME(shl),
    OP_HANDLER_NAME(shr),
    OP_HANDLER_NAME(jmp),
    OP_HANDLER_NAME(jz),
    OP_HANDLER_NAME(jd),
    OP_HANDLER_NAME(sti),
    OP_HANDLER_NAME(stib),
    OP_HANDLER_NAME(str),
    OP_HANDLER_NAME(strb),
    OP_HANDLER_NAME(sys),
};

// clang-format on

squirm_op_t squirm_cpu_decode_op(squirm_cpu_t* cpu) {
    squirm_op_t op;
    u16 ip = cpu->reg[BURROW_REG_IP];

    op.op = cpu->mem[ip++];
    op.args.dest = cpu->mem[ip++];
    op.args.src_a = cpu->mem[ip++];
    op.args.src_b = cpu->mem[ip++];

    return op;
}

void squirm_cpu_step(squirm_cpu_t* cpu) {
    if (cpu->reg[BURROW_REG_IP] % 4 != 0) {
        LOG_ERROR("unaligned instruction pointer %04x\n", cpu->reg[BURROW_REG_IP]);
        squirm_cpu_set_flag(cpu, BURROW_FL_FIN);
        return;
    }

    squirm_op_t op = squirm_cpu_decode_op(cpu);

    if (op.op >= BURROW_OP_COUNT) {
        LOG_ERROR("invalid opcode %02x at %04x\n", op.op, cpu->reg[BURROW_REG_IP]);
        squirm_cpu_set_flag(cpu, BURROW_FL_FIN);
        return;
    }
    cpu->reg[BURROW_REG_IP] += 4;
    cpu->executed_op_count++;
    k_op_handlers[op.op](cpu, op);
}
