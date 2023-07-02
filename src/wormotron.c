#include "SDL_mutex.h"
#include "graphics.h"
#include "log.h"
#include "squirm.h"
#include "types.h"
#include "rom.h"
#include "wormotron.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

bool g_stop = false;
bool g_wait_for_present = false;
wormotron_t* g_wormotron = NULL;

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

static void syscall_wait_for_present(squirm_cpu_t* cpu) {
    (void)cpu; // unused
    g_wait_for_present = true;
}

static void mmio_instant_putc_write(u16 addr, u8 val) {
    (void)addr; // unused

    putc(val, stdout);
    fflush(stdout);
}

static void mmio_graphics_write(u16 addr, u8 val) {
    wormotron_graphics_write(g_wormotron->graphics, addr - WT_GRAPHICS_RAM_START, val);
}

// clang-format off
static squirm_mmio_entry_t k_mmio[] = {
    {
        .start = 0xff00,
        .end = 0xff01,
        .write = mmio_instant_putc_write,
        .read = NULL
    },
    {
        .start = WT_GRAPHICS_RAM_START,
        .end = WT_GRAPHICS_RAM_START + WT_GRAPHICS_RAM_SIZE,
        .write = mmio_graphics_write,
        .read = NULL
    },
};
// clang-format on

wormotron_t* wormotron_new(const char* rom_file) {
    LOG_DEBUG("Initializing wormotron...\n");
    wormotron_t* wormotron = malloc(sizeof(wormotron_t));

    wormotron->rom = wormotron_rom_load(rom_file);
    wormotron->cpu = squirm_cpu_new(
        (squirm_cpu_syscall_fn[]){
            syscall_exit,
            syscall_print,
            syscall_wait_for_present,
        },
        3
    );

    for (usize i = 0; i < sizeof(k_mmio) / sizeof(k_mmio[0]); i++) {
        squirm_cpu_add_mmio_entry(wormotron->cpu, k_mmio[i]);
    }

    LOG_DEBUG("MMIO entries: %d\n", wormotron->cpu->mmio_count);

    squirm_cpu_load(wormotron->cpu, wormotron->rom->data, wormotron->rom->size);

    squirm_cpu_reset(wormotron->cpu);

    wormotron->graphics = wormotron_graphics_new();

    return wormotron;
}

void wormotron_free(wormotron_t* wormotron) {
    squirm_cpu_free(wormotron->cpu);
    wormotron_rom_free(wormotron->rom);
    free(wormotron);
}

// static int wormotron_cpu_thread(void* data) {
//     wormotron_t* wormotron = (wormotron_t*)data;
//
//     while (!g_stop) {
//         squirm_cpu_step(wormotron->cpu);
//         if (wormotron->cpu->reg[BURROW_REG_FL] & BURROW_FL_FIN) {
//             break;
//         }
//     }
//
//     g_stop = true;
//
//     return 0;
// }

void wormotron_run(wormotron_t* wormotron) {
    LOG_INFO("Starting wormotron...\n");

    // Start the CPU thread.
    // SDL_Thread* cpu_thread =
    //     SDL_CreateThread(wormotron_cpu_thread, "wormotron_cpu_thread", wormotron);
    //
    // if (cpu_thread == NULL) {
    //     LOG_ERROR("Failed to create CPU thread: %s\n", SDL_GetError());
    //     exit(1);
    // }

    LOG_DEBUG("CPU thread started.\n");

    // Graphics loop.

    while (!g_stop) {
        // Update the game state.
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                g_stop = true;
            }
        }

        u32 start = SDL_GetTicks();

        while (SDL_GetTicks() - start < 16) {
            squirm_cpu_step(wormotron->cpu);
            if (wormotron->cpu->reg[BURROW_REG_FL] & BURROW_FL_FIN) {
                g_stop = true;
                break;
            }
            if (g_wait_for_present) {
                g_wait_for_present = false;
                break;
            }
        }

        wormotron_graphics_clear(wormotron->graphics);

        wormotron_graphics_flush(wormotron->graphics);

        wormotron_graphics_present(wormotron->graphics);
    }

    LOG_INFO("Exiting wormotron...\n");

    // SDL_WaitThread(cpu_thread, NULL);
}
