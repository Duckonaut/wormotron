#pragma once

#include <stdio.h>
#include "burrow.h"
#include "lexer.h"
#include "preprocessor.h"
#include "types.h"

typedef struct burrow_op {
    u8 op;
    struct {
        u8 dest;
        u8 src_a;
        u8 src_b;
    } regs;
} burrow_op_t;

typedef struct weave_label {
    char* name;
    u16 addr;
    bool defined;

    u16* unresolved_refs;
    usize unresolved_refs_len;
    usize unresolved_refs_cap;
} weave_label_t;

#define WEAVE_MAX_LABELS 256

typedef struct weave {
    FILE* output;
    weave_preprocessor_t* preprocessor;

    // state
    weave_token_t token;
    u16 addr;

    // labels
    // TODO: use a hash table
    weave_label_t labels[WEAVE_MAX_LABELS];
    u16 num_labels;

    bool at_eof;
} weave_t;

void weave_process(FILE* input, FILE* output);
