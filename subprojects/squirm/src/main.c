#define _POSIX_C_SOURCE 199309L
#include "burrow.h"
#include "log.h"
#include "types.h"
#include "squirm.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <time.h>

typedef struct args {
    char* rom_file;
} args_t;

static void usage(void) {
    printf("Usage: squirm <rom_file>\n");
}

static void parse_args(int argc, char* argv[], args_t* args) {
    if (argc < 2 || argc > 5) {
        usage();
        exit(1);
    }

    args->rom_file = argv[1];
}

static void syscall_exit(squirm_cpu_t* cpu) {
    cpu->reg[BURROW_REG_FL] |= BURROW_FL_FIN;
}

static void syscall_print(squirm_cpu_t* cpu) {
    u16 addr = cpu->reg[BURROW_REG_B];
    u16 len = cpu->reg[BURROW_REG_C];

    for (int i = 0; i < len; i++) {
        putc(cpu->mem[addr + i], stdout);
    }

    fflush(stdout);

    cpu->reg[BURROW_REG_A] = 0;
}

int main(int argc, char* argv[]) {
    // init logging

    log_init();

    args_t args;
    parse_args(argc, argv, &args);

    FILE* rom_file = fopen(args.rom_file, "r");

    if (rom_file == NULL) {
        LOG_ERROR("Failed to open rom file: %s\n", args.rom_file);
        exit(1);
    }

    fseek(rom_file, 0, SEEK_END);

    u32 rom_size = ftell(rom_file);

    fseek(rom_file, 0, SEEK_SET);

    if (rom_size > 0x10000) {
        LOG_ERROR("Rom file too large: %s\n", args.rom_file);
        exit(1);
    }

    LOG_DEBUG("Rom size: %d\n", rom_size);

    u8* rom = malloc(rom_size);

    if (rom == NULL) {
        LOG_ERROR("Failed to allocate memory for rom: %s\n", args.rom_file);
        exit(1);
    }

    fread(rom, 1, rom_size, rom_file);

    fclose(rom_file);

    squirm_cpu_t* cpu =
        squirm_cpu_new((squirm_cpu_syscall_fn[]){ syscall_exit, syscall_print }, 2);
    squirm_cpu_load(cpu, rom, rom_size);

    squirm_cpu_reset(cpu);

#ifndef _WIN32
    struct timeval start, end;

    gettimeofday(&start, NULL);
#endif

    while (1) {
        squirm_cpu_step(cpu);
        if (cpu->reg[BURROW_REG_FL] & BURROW_FL_FIN) {
            break;
        }
    }

#ifndef _WIN32
    gettimeofday(&end, NULL);

    // get elapsed microseconds
    u64 elapsed = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

    LOG_INFO(
        "Executed %ld instructions in %ld microseconds\n",
        cpu->executed_op_count,
        elapsed
    );
    LOG_INFO("Calculated frequency: %ld Hz\n", cpu->executed_op_count * 1000000 / elapsed);
#endif

    free(rom);

    log_close();

    return 0;
}
