#include "weave.h"
#include "preprocessor.h"
#include "weave.h"
#include "lexer.h"
#include "log.h"
#include "types.h"

#include <string.h>
#include <stdio.h>

#define TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(ty)                                                  \
    case WEAVE_TOKEN_##ty:                                                                     \
        fprintf(output, #ty "\n");                                                             \
        break;

void weave_process(FILE* input, FILE* output) {
    weave_lexer_t lexer;
    weave_lexer_init(&lexer, input);

    weave_preprocessor_t* preprocessor = weave_preprocessor_new(&lexer);

    weave_preprocessor_result_t result = weave_preprocessor_next(preprocessor);

    if (!result.is_ok) {
        LOG_ERROR(
            "preprocessor error at %d:%d: %s",
            lexer.line,
            lexer.col,
            weave_preprocessor_error_str(result.err)
        );
    } else if (!result.ok.is_ok) {
        LOG_ERROR(
            "lexer error at %d:%d: %s",
            lexer.line,
            lexer.col,
            weave_lexer_error_str(result.ok.err)
        );
    }

    weave_token_t token = result.ok.ok;

    bool at_eof = false;

    while (result.is_ok && token.ty != WEAVE_TOKEN_INVALID && !at_eof) {
        if (!result.ok.is_ok) {
            LOG_ERROR("lexer error: %s", weave_lexer_error_str(result.ok.err));
            break;
        }
        if (result.is_ok) {
            token = result.ok.ok;
            switch (token.ty) {
                case WEAVE_TOKEN_INVALID:
                    fprintf(stderr, "Invalid token\n");
                    break;
                case WEAVE_TOKEN_EOF:
                    fprintf(output, "EOF\n");
                    at_eof = true;
                    break;
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(NEWLINE)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(COMMA)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(COLON)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(PERCENT)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(DOLLAR)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(DOT)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(BANG)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(MINUS)
                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(SEMICOLON)

                    TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(MACRO)

                case WEAVE_TOKEN_IDENTIFIER:
                    fprintf(output, "IDENTIFIER: ");
                    fwrite(token.val.str_val.val, 1, token.val.str_val.len, output);
                    fprintf(output, "\n");
                    break;
                case WEAVE_TOKEN_INT:
                    fprintf(output, "INT: %d\n", token.val.int_val);
                    break;
                case WEAVE_TOKEN_STR:
                    fprintf(output, "STR: \"");
                    fwrite(token.val.str_val.val, 1, token.val.str_val.len, output);
                    fprintf(output, "\"\n");
                    break;
                case WEAVE_TOKEN_CHAR:
                    fprintf(output, "CHAR: '");
                    fwrite(token.val.str_val.val, 1, token.val.str_val.len, output);
                    fprintf(output, "'\n");
                    break;
            }
        } else {
            LOG_ERROR(
                "preprocessor error at %d:%d: %s",
                lexer.line,
                lexer.col,
                weave_lexer_error_str(result.ok.err)
            );
        }

        weave_token_free(&token);
        result = weave_preprocessor_next(preprocessor);
    }

    if (!result.is_ok) {
        LOG_ERROR(
            "preprocessor error at %d:%d: %s",
            lexer.line,
            lexer.col,
            weave_preprocessor_error_str(result.err)
        );
    }

    weave_token_free(&token);

    weave_preprocessor_free(preprocessor);
}
