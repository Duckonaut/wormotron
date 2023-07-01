#include "weave.h"
#include "preprocessor.h"
#include "weave.h"
#include "lexer.h"
#include "log.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void weave_advance(weave_t* weave) {
    weave_preprocessor_result_t result = weave_preprocessor_next(weave->preprocessor);

    if (!result.is_ok) {
        LOG_ERROR(
            "preprocessor error at %d:%d: %s",
            weave->preprocessor->lexer->line,
            weave->preprocessor->lexer->col,
            weave_preprocessor_error_str(result.err)
        );
        exit(1);
    } else if (!result.ok.is_ok) {
        LOG_ERROR(
            "lexer error at %d:%d: %s",
            weave->preprocessor->lexer->line,
            weave->preprocessor->lexer->col,
            weave_lexer_error_str(result.ok.err)
        );
        exit(1);
    } else {
        weave->token = result.ok.ok;

        weave->at_eof = weave->token.ty == WEAVE_TOKEN_EOF;
    }
}

static weave_t weave_new(weave_preprocessor_t* preprocessor, FILE* output) {
    weave_t weave;
    weave.preprocessor = preprocessor;
    weave.output = output;

    weave.at_eof = false;

    weave_advance(&weave);

    return weave;
}

static void weave_free(weave_t* weave) {
    weave_preprocessor_free(weave->preprocessor);
}

#define TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(ty)                                                  \
    case WEAVE_TOKEN_##ty:                                                                     \
        fprintf(stdout, #ty "\n");                                                             \
        break;

static void weave_run(weave_t* weave) {
    while (!weave->at_eof) {
        weave_advance(weave);

        switch (weave->token.ty) {
            case WEAVE_TOKEN_INVALID: {
                LOG_ERROR(
                    "lexer error at %d:%d: Invalid token\n",
                    weave->preprocessor->lexer->line,
                    weave->preprocessor->lexer->col
                );
                exit(1);
                break;
            }
            case WEAVE_TOKEN_EOF:
                weave->at_eof = true;
                fprintf(stdout, "EOF\n");
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
                fprintf(stdout, "IDENTIFIER: ");
                fwrite(weave->token.val.str_val.val, 1, weave->token.val.str_val.len, stdout);
                fprintf(stdout, "\n");
                break;
            case WEAVE_TOKEN_INT:
                fprintf(stdout, "INT: %d\n", weave->token.val.int_val);
                break;
            case WEAVE_TOKEN_STR:
                fprintf(stdout, "STR: \"");
                fwrite(weave->token.val.str_val.val, 1, weave->token.val.str_val.len, stdout);
                fprintf(stdout, "\"\n");
                break;
            case WEAVE_TOKEN_CHAR:
                fprintf(stdout, "CHAR: \'%c\'\n", weave->token.val.char_val);
                break;
        }
    }
}

void weave_process(FILE* input, FILE* output) {
    weave_lexer_t* lexer = weave_lexer_new(input);

    weave_preprocessor_t* preprocessor = weave_preprocessor_new(lexer);

    weave_t weave = weave_new(preprocessor, output);

    weave_run(&weave);

    weave_free(&weave);
}
