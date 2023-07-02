#pragma once

#include "lexer.h"
#include "types.h"
#include "result.h"

typedef struct weave_macro {
    usize len_args;
    usize cap_args;
    char** arg_names;
    usize len_tokens;
    usize cap_tokens;
    weave_token_t* tokens;
} weave_macro_t;

void weave_macro_free(weave_macro_t* macro);

typedef struct weave_preprocessor {
    weave_lexer_t* lexer;
    weave_token_pos_t macro_start_pos;
    weave_token_t current_lexer_token;
    char** macro_names;
    weave_macro_t** macros;
    usize num_macros;
    usize cap_macros;

    weave_macro_t* current_macro;
    struct {
        weave_token_t* arg_tokens;
        usize arg_token_len;
        usize arg_token_cap;
    }* current_macro_args;
    usize current_macro_arg_count;
    usize current_macro_token_index;
    usize current_macro_arg_index;
    usize current_macro_arg_expansion_index;
    bool in_macro_arg_expansion;

    bool at_eof;
} weave_preprocessor_t;

typedef enum weave_preprocessor_error {
    WEAVE_PREPROCESSOR_ERROR_NONE,
    WEAVE_PREPROCESSOR_ERROR_UNKNOWN_DIRECTIVE,
    WEAVE_PREPROCESSOR_ERROR_INVALID_MACRO_ARG_NAME,
    WEAVE_PREPROCESSOR_ERROR_INVALID_MACRO_ARG_COUNT,
} weave_preprocessor_error_t;

const char* weave_preprocessor_error_str(weave_preprocessor_error_t error);

weave_preprocessor_t* weave_preprocessor_new(weave_lexer_t* lexer);
void weave_preprocessor_free(weave_preprocessor_t* preprocessor);
weave_token_t weave_preprocessor_next(weave_preprocessor_t* preprocessor);
