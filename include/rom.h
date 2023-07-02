#pragma once

#include "types.h"

typedef struct wormotron_rom {
    u8* data;
    u16 size;
} wormotron_rom_t;

wormotron_rom_t* wormotron_rom_load(const char* rom_file);
void wormotron_rom_free(wormotron_rom_t* rom);
