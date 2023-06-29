#pragma once

#include <stdio.h>
#include "burrow.h"
#include "types.h"

typedef struct burrow_op {
    u8 op;
    struct {
        u8 dest;
        u8 src_a;
        u8 src_b;
    } regs;
} burrow_op_t;

void weave_process(FILE* input, FILE* output);
