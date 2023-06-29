#include "lexer.h"

#include "types.h"
#include <stdlib.h>
#include <string.h>

const char* weave_lexer_error_str(weave_lexer_error_t error) {
    switch (error) {
        case WEAVE_LEXER_ERROR_NONE:
            return "No error";
        case WEAVE_LEXER_ERROR_INVALID_CHAR:
            return "Invalid character";
        case WEAVE_LEXER_ERROR_UNTERMINATED_STRING:
            return "Unterminated string";
        case WEAVE_LEXER_ERROR_UNTERMINATED_CHAR:
            return "Unterminated character";
        case WEAVE_LEXER_ERROR_INVALID_INT:
            return "Invalid integer";
        case WEAVE_LEXER_ERROR_INVALID_HEX:
            return "Invalid hexadecimal";
        case WEAVE_LEXER_ERROR_INVALID_BIN:
            return "Invalid binary";
        case WEAVE_LEXER_ERROR_INVALID_ESCAPE:
            return "Invalid escape sequence";
        case WEAVE_LEXER_ERROR_INVALID_KEYWORD:
            return "Invalid keyword";
        case WEAVE_LEXER_ERROR_STRING_TOO_LONG:
            return "String too long";
        case WEAVE_LEXER_ERROR_CHAR_TOO_LONG:
            return "Character too long";
        case WEAVE_LEXER_ERROR_INT_TOO_LARGE:
            return "Integer too large";
        case WEAVE_LEXER_ERROR_IDENTIFIER_TOO_LONG:
            return "Identifier too long";
        default:
            return "Unknown error";
    }
}

void weave_token_free(weave_token_t* token) {
    if (token->ty == WEAVE_TOKEN_IDENTIFIER || token->ty == WEAVE_TOKEN_STR) {
        free(token->val.str_val.val);
    }
}

weave_token_t weave_token_clone(weave_token_t *token) {
    if (token->ty == WEAVE_TOKEN_IDENTIFIER || token->ty == WEAVE_TOKEN_STR) {
        return (weave_token_t) {
            .ty = token->ty,
            .val = {
                .str_val = {
                    .val = strndup(token->val.str_val.val, token->val.str_val.len),
                    .len = token->val.str_val.len
                }
            }
        };
    } else {
        return *token;
    }
}

void weave_lexer_init(weave_lexer_t* lexer, FILE* input) {
    lexer->input = input;
    lexer->line = 1;
    lexer->col = 1;
    lexer->pos = 0;
    lexer->c = fgetc(input);
}

void weave_lexer_free(weave_lexer_t* lexer) {
    (void)lexer; // unused
    // nothing to do
}

static void weave_lexer_advance(weave_lexer_t* lexer) {
    if (lexer->c == '\n') {
        lexer->line++;
        lexer->col = 0;
    }

    lexer->c = fgetc(lexer->input);
    lexer->pos++;
    lexer->col++;
}

static void weave_lexer_skip_meaningless_whitespace(weave_lexer_t* lexer) {
    while (lexer->c == ' ' || lexer->c == '\t') {
        weave_lexer_advance(lexer);
    }

    if (lexer->c == '#') {
        while (lexer->c != '\n') {
            weave_lexer_advance(lexer);
        }
    }
}

static weave_lexer_result_t weave_lexer_parse_string(weave_lexer_t* lexer) {
    weave_token_t token;
    token.ty = WEAVE_TOKEN_STR;
    token.pos.line = lexer->line;
    token.pos.col = lexer->col;

    char* buffer = malloc(1024);
    size_t buffer_size = 1024;
    size_t buffer_pos = 0;

    while (lexer->c != '"') {
        if (lexer->c == EOF) {
            weave_lexer_result_t result;
            result.is_ok = false;
            result.err = WEAVE_LEXER_ERROR_UNTERMINATED_STRING;
            return result;
        } else if (lexer->c == '\\') {
            weave_lexer_advance(lexer);

            switch (lexer->c) {
                case 'n':
                    buffer[buffer_pos++] = '\n';
                    break;
                case 't':
                    buffer[buffer_pos++] = '\t';
                    break;
                case 'r':
                    buffer[buffer_pos++] = '\r';
                    break;
                case '\\':
                    buffer[buffer_pos++] = '\\';
                    break;
                case '"':
                    buffer[buffer_pos++] = '"';
                    break;
                default: {
                    weave_lexer_result_t result;
                    result.is_ok = false;
                    result.err = WEAVE_LEXER_ERROR_INVALID_ESCAPE;
                    return result;
                }
            }
        }

        if (buffer_pos >= buffer_size) {
            weave_lexer_result_t result;
            result.is_ok = false;
            result.err = WEAVE_LEXER_ERROR_STRING_TOO_LONG;
            return result;
        }

        buffer[buffer_pos++] = lexer->c;
        weave_lexer_advance(lexer);
    }

    token.val.str_val.val = buffer;
    token.val.str_val.len = buffer_pos;

    weave_lexer_advance(lexer);

    weave_lexer_result_t result;
    result.is_ok = true;
    result.ok = token;

    return result;
}

static weave_lexer_result_t weave_lexer_parse_char(weave_lexer_t* lexer) {
    weave_token_t token;
    token.ty = WEAVE_TOKEN_CHAR;
    token.pos.line = lexer->line;
    token.pos.col = lexer->col;

    weave_lexer_advance(lexer);

    if (lexer->c == '\\') {
        weave_lexer_advance(lexer);

        switch (lexer->c) {
            case 'n':
                token.val.char_val = '\n';
                break;
            case 't':
                token.val.char_val = '\t';
                break;
            case 'r':
                token.val.char_val = '\r';
                break;
            case '\\':
                token.val.char_val = '\\';
                break;
            case '\'':
                token.val.char_val = '\'';
                break;
            default: {
                weave_lexer_result_t result;
                result.is_ok = false;
                result.err = WEAVE_LEXER_ERROR_INVALID_ESCAPE;
                return result;
            }
        }
    } else {
        token.val.char_val = lexer->c;
    }

    weave_lexer_advance(lexer);

    if (lexer->c != '\'') {
        weave_lexer_result_t result;
        result.is_ok = false;
        result.err = WEAVE_LEXER_ERROR_UNTERMINATED_CHAR;
        return result;
    }

    weave_lexer_advance(lexer);

    weave_lexer_result_t result;
    result.is_ok = true;
    result.ok = token;

    return result;
}

static weave_lexer_result_t weave_lexer_parse_int(weave_lexer_t* lexer) {
    weave_token_t token;
    token.ty = WEAVE_TOKEN_INT;
    token.pos.line = lexer->line;
    token.pos.col = lexer->col;

    bool negative = false;

    if (lexer->c == '-') {
        negative = true;
        weave_lexer_advance(lexer);
    }

    u32 val = 0;
    // handle leading zeros, hex and binary

    if (lexer->c == '0') {
        weave_lexer_advance(lexer);

        if (lexer->c == 'x') {
            weave_lexer_advance(lexer);

            while (true) {
                if (lexer->c >= '0' && lexer->c <= '9') {
                    val = val * 16 + (lexer->c - '0');
                } else if (lexer->c >= 'a' && lexer->c <= 'f') {
                    val = val * 16 + (lexer->c - 'a' + 10);
                } else if (lexer->c >= 'A' && lexer->c <= 'F') {
                    val = val * 16 + (lexer->c - 'A' + 10);
                } else {
                    break;
                }

                if (val > 0xFFFF) {
                    weave_lexer_result_t result;
                    result.is_ok = false;
                    result.err = WEAVE_LEXER_ERROR_INT_TOO_LARGE;
                    return result;
                }

                weave_lexer_advance(lexer);
            }

            token.val.int_val = val;
            if (negative) {
                token.val.int_val = -val;
            }

            weave_lexer_result_t result;
            result.is_ok = true;
            result.ok = token;

            return result;
        } else if (lexer->c == 'b') {
            weave_lexer_advance(lexer);

            while (true) {
                if (lexer->c == '0' || lexer->c == '1') {
                    val = val * 2 + (lexer->c - '0');
                } else {
                    break;
                }

                if (val > 0xFFFF) {
                    weave_lexer_result_t result;
                    result.is_ok = false;
                    result.err = WEAVE_LEXER_ERROR_INT_TOO_LARGE;
                    return result;
                }

                weave_lexer_advance(lexer);
            }

            token.val.int_val = val;
            if (negative) {
                token.val.int_val = -val;
            }

            weave_lexer_result_t result;
            result.is_ok = true;
            result.ok = token;

            return result;
        }
    }

    while (lexer->c >= '0' && lexer->c <= '9') {
        val = val * 10 + (lexer->c - '0');

        if (val > 0xFFFF) {
            weave_lexer_result_t result;
            result.is_ok = false;
            result.err = WEAVE_LEXER_ERROR_INT_TOO_LARGE;
            return result;
        }

        weave_lexer_advance(lexer);
    }

    token.val.int_val = val;
    if (negative) {
        token.val.int_val = -val;
    }

    weave_lexer_result_t result;
    result.is_ok = true;
    result.ok = token;

    return result;
}

static weave_token_ty_t weave_lexer_keyword(const char* buffer) {
    if (strcmp(buffer, "macro") == 0) {
        return WEAVE_TOKEN_MACRO;
    }

    return WEAVE_TOKEN_INVALID;
}

static weave_lexer_result_t weave_lexer_parse_identifier(weave_lexer_t* lexer) {
    weave_token_t token;
    token.pos.line = lexer->line;
    token.pos.col = lexer->col;

    char* buffer = malloc(1024);
    size_t buffer_size = 1024;
    size_t buffer_pos = 0;

    // first char must be [a-zA-Z_]
    // rest can be [a-zA-Z0-9_]

    if (!((lexer->c >= 'a' && lexer->c <= 'z') || (lexer->c >= 'A' && lexer->c <= 'Z') ||
          lexer->c == '_')) {
        weave_lexer_result_t result;
        result.is_ok = false;
        result.err = WEAVE_LEXER_ERROR_INVALID_CHAR;
        return result;
    }

    while ((lexer->c >= 'a' && lexer->c <= 'z') || (lexer->c >= 'A' && lexer->c <= 'Z') ||
           (lexer->c >= '0' && lexer->c <= '9') || lexer->c == '_') {
        if (buffer_pos >= buffer_size) {
            weave_lexer_result_t result;
            result.is_ok = false;
            result.err = WEAVE_LEXER_ERROR_IDENTIFIER_TOO_LONG;
            return result;
        }

        buffer[buffer_pos++] = lexer->c;
        weave_lexer_advance(lexer);
    }

    buffer[buffer_pos] = '\0';

    token.ty = weave_lexer_keyword(buffer);

    if (token.ty == WEAVE_TOKEN_INVALID) {
        token.ty = WEAVE_TOKEN_IDENTIFIER;
        token.val.str_val.val = buffer;
        token.val.str_val.len = buffer_pos;

        weave_lexer_result_t result;
        result.is_ok = true;
        result.ok = token;
        return result;
    } else {
        free(buffer);
        weave_lexer_result_t result;
        result.is_ok = true;
        result.ok = token;
        return result;
    }
}

static weave_lexer_result_t weave_lexer_make_token(weave_token_ty_t ty, weave_lexer_t* lexer) {
    weave_token_t token;
    token.ty = ty;
    token.pos.line = lexer->line;
    token.pos.col = lexer->col;

    weave_lexer_result_t result;
    result.is_ok = true;
    result.ok = token;

    return result;
}

static weave_lexer_result_t weave_lexer_single_char_token(weave_lexer_t* lexer) {
    switch (lexer->c) {
        case '\n':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_NEWLINE, lexer);
        case ',':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_COMMA, lexer);
        case ':':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_COLON, lexer);
        case '%':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_PERCENT, lexer);
        case '$':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_DOLLAR, lexer);
        case '.':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_DOT, lexer);
        case '!':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_BANG, lexer);
        case '-':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_MINUS, lexer);
        case ';':
            weave_lexer_advance(lexer);
            return weave_lexer_make_token(WEAVE_TOKEN_SEMICOLON, lexer);
        default: {
            weave_lexer_result_t result;
            result.is_ok = false;
            result.err = WEAVE_LEXER_ERROR_INVALID_CHAR;
            return result;
        }
    }
}

weave_lexer_result_t weave_lexer_next(weave_lexer_t* lexer) {
    weave_lexer_skip_meaningless_whitespace(lexer);

    weave_lexer_result_t result;

    if (lexer->c == '\0' || lexer->c == EOF) {
        weave_token_t token;
        token.ty = WEAVE_TOKEN_EOF;
        token.pos.line = lexer->line;
        token.pos.col = lexer->col;

        result.is_ok = true;
        result.ok = token;
        return result;
    }

    weave_lexer_result_t single_result = weave_lexer_single_char_token(lexer);

    if (single_result.is_ok) {
        return single_result;
    }

    if (lexer->c == '"') {
        weave_lexer_advance(lexer);

        weave_lexer_result_t string_result = weave_lexer_parse_string(lexer);

        return string_result;
    }

    if (lexer->c == '\'') {
        weave_lexer_advance(lexer);

        weave_lexer_result_t char_result = weave_lexer_parse_char(lexer);

        return char_result;
    }

    if ((lexer->c >= '0' && lexer->c <= '9') || lexer->c == '-') {
        weave_lexer_result_t number_result = weave_lexer_parse_int(lexer);

        return number_result;
    }

    if ((lexer->c >= 'a' && lexer->c <= 'z') || (lexer->c >= 'A' && lexer->c <= 'Z') ||
        lexer->c == '_') {
        weave_lexer_result_t keyword_result = weave_lexer_parse_identifier(lexer);

        return keyword_result;
    }

    weave_lexer_result_t err_result;
    err_result.is_ok = false;
    err_result.err = WEAVE_LEXER_ERROR_INVALID_CHAR;
    return err_result;
}
