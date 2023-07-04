#include "utils.h"

#include "types.h"

#include <stdio.h>

void dump_buffer(const u8* buffer, usize buffer_size, usize width) {
    printf("buffer_size: %ld\n", buffer_size);
    printf("buffer: [\n");
    printf("0000:");
    for (usize i = 0; i < buffer_size; i++) {
        printf("\t0x%02x,", buffer[i]);
        if (i % width == width - 1) {
            printf("\n%04lx:", i + 1);
        }
    }
    printf("\n]\n");
}
