#pragma once
#include "result.h"
#include "types.h"

#include <stdio.h>

typedef enum weave_token_ty {
    WEAVE_TOKEN_INVALID,
    WEAVE_TOKEN_EOF,
    // single-char tokens
    WEAVE_TOKEN_NEWLINE,
    WEAVE_TOKEN_COMMA,
    WEAVE_TOKEN_COLON,
    WEAVE_TOKEN_PERCENT,
    WEAVE_TOKEN_DOLLAR,
    WEAVE_TOKEN_DOT,
    WEAVE_TOKEN_BANG,
    WEAVE_TOKEN_MINUS,
    WEAVE_TOKEN_SEMICOLON,

    // keywords
    WEAVE_TOKEN_MACRO,

    // value-holding tokens
    WEAVE_TOKEN_IDENTIFIER, // [a-zA-Z_][a-zA-Z0-9_]*
    WEAVE_TOKEN_INT,        // 123, 0x123, 0b101, -123, -0x123, -0b101
    WEAVE_TOKEN_STR,        // "string"
    WEAVE_TOKEN_CHAR,       // 'c'
} weave_token_ty_t;

typedef union weave_token_val {
    i32 int_val;
    struct {
        u16 len;
        char* val;
    } str_val;
    char char_val;
} weave_token_val_t;

typedef struct weave_token_pos {
    u32 line;
    u32 col;
} weave_token_pos_t;

typedef struct weave_token {
    weave_token_ty_t ty;
    weave_token_val_t val;
    weave_token_pos_t pos;
} weave_token_t;

void weave_token_free(weave_token_t* token);
weave_token_t weave_token_clone(weave_token_t* token);

typedef struct weave_lexer {
    FILE* input; // read stream
    usize pos;   // current position in the stream
    u32 line;
    u32 col;
    char c; // current character
} weave_lexer_t;

typedef enum weave_lexer_error {
    WEAVE_LEXER_ERROR_NONE,
    WEAVE_LEXER_ERROR_INVALID_CHAR,
    WEAVE_LEXER_ERROR_UNTERMINATED_STRING,
    WEAVE_LEXER_ERROR_UNTERMINATED_CHAR,
    WEAVE_LEXER_ERROR_INVALID_INT,
    WEAVE_LEXER_ERROR_INVALID_HEX,
    WEAVE_LEXER_ERROR_INVALID_BIN,
    WEAVE_LEXER_ERROR_INVALID_ESCAPE,
    WEAVE_LEXER_ERROR_INVALID_KEYWORD,
    WEAVE_LEXER_ERROR_STRING_TOO_LONG,
    WEAVE_LEXER_ERROR_CHAR_TOO_LONG,
    WEAVE_LEXER_ERROR_INT_TOO_LARGE,
    WEAVE_LEXER_ERROR_IDENTIFIER_TOO_LONG,
} weave_lexer_error_t;

const char* weave_lexer_error_str(weave_lexer_error_t error);

typedef RESULT_TYPE(weave_token_t, weave_lexer_error_t) weave_lexer_result_t;

void weave_lexer_init(weave_lexer_t* lexer, FILE* input);
void weave_lexer_free(weave_lexer_t* lexer);

weave_lexer_result_t weave_lexer_next(weave_lexer_t* lexer);
