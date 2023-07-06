#include "squirm_dbg.h"
#include "burrow.h"
#include "log.h"
#include "squirm.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void squirm_dbg_breakpoint_add(squirm_dbg_t* dbg, u16 addr) {
    dbg->breakpoints[dbg->breakpoint_count++].addr = addr;
}

static void squirm_dbg_watchpoint_add(squirm_dbg_t* dbg, u16 addr) {
    dbg->watchpoints[dbg->watchpoint_count++].addr = addr;
}

static void squirm_dbg_breakpoint_remove(squirm_dbg_t* dbg, u16 addr) {
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].addr == addr) {
            dbg->breakpoints[i] = dbg->breakpoints[--dbg->breakpoint_count];
            return;
        }
    }
}

static void squirm_dbg_watchpoint_remove(squirm_dbg_t* dbg, u16 addr) {
    for (int i = 0; i < dbg->watchpoint_count; i++) {
        if (dbg->watchpoints[i].addr == addr) {
            dbg->watchpoints[i] = dbg->watchpoints[--dbg->watchpoint_count];
            return;
        }
    }
}

static bool squirm_dbg_breakpoint_check(squirm_dbg_t* dbg, u16 addr) {
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].addr == addr) {
            return true;
        }
    }

    return false;
}

static bool squirm_dbg_watchpoint_check(squirm_dbg_t* dbg, u16 addr) {
    for (int i = 0; i < dbg->watchpoint_count; i++) {
        if (dbg->watchpoints[i].addr == addr) {
            return true;
        }
    }

    return false;
}

squirm_dbg_t* squirm_dbg_new(squirm_cpu_t* cpu) {
    squirm_dbg_t* dbg = malloc(sizeof(squirm_dbg_t));
    dbg->cpu = cpu;
    memset(dbg->breakpoints, 0, sizeof(dbg->breakpoints));
    dbg->breakpoint_count = 0;
    memset(dbg->watchpoints, 0, sizeof(dbg->watchpoints));
    dbg->watchpoint_count = 0;

    memset(dbg->regwatch, 0, sizeof(dbg->regwatch));

    memcpy(dbg->prev_regs, cpu->reg, sizeof(cpu->reg));

    squirm_dbg_breakpoint_add(dbg, 0x0000);

    dbg->running = false;

    return dbg;
}

void squirm_dbg_free(squirm_dbg_t* dbg) {
    free(dbg);
}

static inline u16 squirm_op_imm(squirm_op_t op) {
    return (op.args.src_a << 8) | (op.args.src_b & 0xFF);
}

static inline u8 squirm_cpu_read8(squirm_cpu_t* cpu, u16 addr) {
    return cpu->mem[addr];
}

static inline u16 squirm_cpu_read16(squirm_cpu_t* cpu, u16 addr) {
    return cpu->mem[addr] | (cpu->mem[addr + 1] << 8);
}

void squirm_dbg_prompt(squirm_dbg_t* dbg) {
    // not running: wait for command
    char cmd[256];
    printf("> ");
    fflush(stdout);

    if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
        LOG_ERROR("Failed to read command\n");
        return;
    }

    char* nl = strchr(cmd, '\n');

    if (nl != NULL) {
        *nl = '\0';
    }

    char* args = strchr(cmd, ' ');

    if (args != NULL) {
        *args++ = '\0';
    }

    char* cmd_name = cmd;

    if (strcmp(cmd_name, "c") == 0 || strcmp(cmd_name, "continue") == 0) {
        dbg->running = true;
        LOG_DEBUG("Continuing...\n");
        squirm_cpu_step(dbg->cpu);
    } else if (strcmp(cmd_name, "s") == 0 || strcmp(cmd_name, "step") == 0) {
        squirm_cpu_step(dbg->cpu);
    } else if (strcmp(cmd_name, "b") == 0 || strcmp(cmd_name, "break") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing breakpoint address\n");
            return;
        }

        u16 addr = strtol(args, NULL, 0);

        squirm_dbg_breakpoint_add(dbg, addr);
    } else if (strcmp(cmd_name, "w") == 0 || strcmp(cmd_name, "watch") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing watchpoint address\n");
            return;
        }

        u16 addr = strtol(args, NULL, 0);

        squirm_dbg_watchpoint_add(dbg, addr);
    } else if (strcmp(cmd_name, "d") == 0 || strcmp(cmd_name, "delete") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing breakpoint address\n");
            return;
        }

        u16 addr = strtol(args, NULL, 0);

        squirm_dbg_breakpoint_remove(dbg, addr);
    } else if (strcmp(cmd_name, "x") == 0 || strcmp(cmd_name, "forget") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing watchpoint address\n");
            return;
        }

        u16 addr = strtol(args, NULL, 0);

        squirm_dbg_watchpoint_remove(dbg, addr);
    } else if (strcmp(cmd_name, "r") == 0 || strcmp(cmd_name, "reg") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing register name\n");
            return;
        }

        u8 reg = burrow_register_from_str(args, strlen(args));

        if (reg == BURROW_REG_COUNT) {
            LOG_ERROR("Invalid register name\n");
            return;
        }

        dbg->regwatch[reg] = true;
    } else if (strcmp(cmd_name, "u") == 0 || strcmp(cmd_name, "unreg") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing register name\n");
            return;
        }

        u8 reg = burrow_register_from_str(args, strlen(args));

        if (reg == BURROW_REG_COUNT) {
            LOG_ERROR("Invalid register name\n");
            return;
        }

        dbg->regwatch[reg] = false;
    } else if (strcmp(cmd_name, "p") == 0 || strcmp(cmd_name, "print") == 0) {
        if (args == NULL) {
            LOG_ERROR("Missing print argument\n");
            return;
        }

        char arg_start = args[0];

        switch (arg_start) {
            case '%': {
                u8 reg = burrow_register_from_str(args + 1, strlen(args + 1));

                if (reg == BURROW_REG_COUNT) {
                    LOG_ERROR("Invalid register name\n");
                    return;
                }

                u16 val = dbg->cpu->reg[reg];

                printf("%%%s = 0x%04X\n", burrow_register_to_str(reg), val);
            } break;
            case '*': {
                u16 addr = strtol(args + 1, NULL, 0);

                u8 val = squirm_cpu_read8(dbg->cpu, addr);

                printf("*0x%04X = 0x%02X\n", addr, val);
            } break;
            case '$': {
                u16 addr = strtol(args + 1, NULL, 0);

                u16 val = squirm_cpu_read16(dbg->cpu, addr);

                printf("$0x%04X = 0x%04X\n", addr, val);
            } break;
            default:
                LOG_ERROR("Invalid print argument\n");
                return;
        }
    } else if (strcmp(cmd_name, "i") == 0 || strcmp(cmd_name, "info") == 0) {
        LOG_INFO("Breakpoints:\n");
        for (u8 i = 0; i < dbg->breakpoint_count; i++) {
            printf("  0x%04X\n", dbg->breakpoints[i].addr);
        }

        LOG_INFO("Watchpoints:\n");
        for (u8 i = 0; i < dbg->watchpoint_count; i++) {
            printf("  0x%04X\n", dbg->watchpoints[i].addr);
        }

        LOG_INFO("Watched registers:\n");
        for (u8 i = 0; i < BURROW_REG_COUNT; i++) {
            if (dbg->regwatch[i]) {
                printf("  %%%s\n", burrow_register_to_str(i));
            }
        }
    } else if (strcmp(cmd_name, "?") == 0 || strcmp(cmd_name, "help") == 0) {
        printf("Commands:\n");
        printf("  s | step          - step one instruction\n");
        printf("  c | continue      - continue execution\n");
        printf("  b | break <addr>  - add breakpoint\n");
        printf("  d | delete <addr> - remove breakpoint\n");
        printf("  w | watch <addr>  - add watchpoint\n");
        printf("  x | forget <addr> - remove watchpoint\n");
        printf("  r | reg <reg>     - watch register\n");
        printf("  u | unreg <reg>   - unwatch register\n");
        printf("  p | print %%<reg>  - print register\n");
        printf("            *<addr> - print byte at address\n");
        printf("            $<addr> - print word at address\n");
        printf("  i | info          - print info\n");
        printf("  ? | help          - print help\n");
        printf("  q | quit          - quit debugger\n");
    } else if (strcmp(cmd_name, "q") == 0 || strcmp(cmd_name, "quit") == 0) {
        exit(0);
    } else {
        LOG_ERROR("Unknown command\n");
    }
}

void squirm_dbg_step(squirm_dbg_t* dbg) {
    if (dbg->running) {
        squirm_op_t op = squirm_cpu_decode_op(dbg->cpu);

        if (op.op == BURROW_OP_STI) {
            u16 dest = squirm_op_imm(op);

            if (squirm_dbg_watchpoint_check(dbg, dest) ||
                squirm_dbg_watchpoint_check(dbg, dest + 1)) {
                dbg->running = false;
                LOG_DEBUG("Watchpoint hit at 0x%04X\n", dest);
                return;
            }
        } else if (op.op == BURROW_OP_STIB) {
            u16 dest = squirm_op_imm(op);

            if (squirm_dbg_watchpoint_check(dbg, dest)) {
                dbg->running = false;
                LOG_DEBUG("Watchpoint hit at 0x%04X\n", dest);
                return;
            }
        } else if (op.op == BURROW_OP_STR) {
            u8 dest_reg = op.args.dest;
            u16 dest = dbg->cpu->reg[dest_reg];

            if (squirm_dbg_watchpoint_check(dbg, dest) ||
                squirm_dbg_watchpoint_check(dbg, dest + 1)) {
                dbg->running = false;
                LOG_DEBUG("Watchpoint hit at 0x%04X\n", dest);
                return;
            }
        } else if (op.op == BURROW_OP_STRB) {
            u8 dest_reg = op.args.dest;
            u16 dest = dbg->cpu->reg[dest_reg];

            if (squirm_dbg_watchpoint_check(dbg, dest)) {
                dbg->running = false;
                LOG_DEBUG("Watchpoint hit at 0x%04X\n", dest);
                return;
            }
        }

        if (squirm_dbg_breakpoint_check(dbg, dbg->cpu->reg[BURROW_REG_IP])) {
            dbg->running = false;
            LOG_DEBUG("Breakpoint hit at 0x%04X\n", dbg->cpu->reg[BURROW_REG_IP]);
            return;
        }

        squirm_cpu_step(dbg->cpu);

        for (u8 i = 0; i < BURROW_REG_COUNT; i++) {
            if (!dbg->regwatch[i]) {
                continue;
            }

            if (dbg->cpu->reg[i] != dbg->prev_regs[i]) {
                u16 from = dbg->prev_regs[i];
                u16 to = dbg->cpu->reg[i];

                const char* reg_name = burrow_register_to_str(i);

                LOG_DEBUG("Register %%%s changed from 0x%04X to 0x%04X\n", reg_name, from, to);
            }
        }

        memcpy(dbg->prev_regs, dbg->cpu->reg, sizeof(dbg->cpu->reg));
    }

    else {
        squirm_dbg_prompt(dbg);
    }
}

void squirm_dbg_run(squirm_dbg_t* dbg) {
    while (true) {
        squirm_dbg_step(dbg);
        if (dbg->cpu->reg[BURROW_REG_FL] & BURROW_FL_FIN) {
            break;
        }
    }
}
