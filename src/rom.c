#include "rom.h"

#include "log.h"
#include "types.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

wormotron_rom_t* wormotron_rom_load(const char* rom_file) {
    FILE* rom_stream = fopen(rom_file, "r");

    if (rom_stream == NULL) {
        LOG_ERROR("Failed to open rom file: %s\n", rom_file);
        exit(1);
    }

    fseek(rom_stream, 0, SEEK_END);

    usize rom_size = (usize)ftell(rom_stream);

    fseek(rom_stream, 0, SEEK_SET);

    if (rom_size > 0x10000) {
        LOG_ERROR("Rom file too large: %s is %zu\n", rom_file, rom_size);
        exit(1);
    }

    LOG_DEBUG("Rom size: %zu\n", rom_size);

    wormotron_rom_t* rom = malloc(sizeof(wormotron_rom_t));

    rom->size = (u16)rom_size;

    if (rom == NULL) {
        LOG_ERROR("Failed to allocate memory for rom: %s\n", rom_file);
        exit(1);
    }

    rom->data = malloc(rom_size);

    if (rom->data == NULL) {
        LOG_ERROR("Failed to allocate memory for rom data: %s\n", rom_file);
        exit(1);
    }

    fread(rom->data, 1, rom_size, rom_stream);

    fclose(rom_stream);

    dump_buffer(rom->data, rom->size, 4);

    return rom;
}

void wormotron_rom_free(wormotron_rom_t* rom) {
    free(rom->data);
    free(rom);
}
