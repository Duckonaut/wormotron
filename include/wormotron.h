#pragma once

#include "SDL_mutex.h"
#include "graphics.h"
#include "squirm.h"
#include "types.h"
#include "rom.h"

#define WT_WINDOW_TITLE "dev: wormotron"
#define WT_WINDOW_LOGICAL_WIDTH 144
#define WT_WINDOW_LOGICAL_HEIGHT 96
#define WT_WINDOW_SCALE 4
#define WT_WINDOW_WIDTH (WT_WINDOW_LOGICAL_WIDTH * WT_WINDOW_SCALE)
#define WT_WINDOW_HEIGHT (WT_WINDOW_LOGICAL_HEIGHT * WT_WINDOW_SCALE)

extern bool g_stop;

typedef struct wormotron {
    wormotron_graphics_t* graphics;
    squirm_cpu_t* cpu;
    wormotron_rom_t* rom;
} wormotron_t;

extern wormotron_t* g_wormotron;

wormotron_t* wormotron_new(const char* rom_file);
void wormotron_free(wormotron_t* wormotron);

void wormotron_run(wormotron_t* wormotron);
