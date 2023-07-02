#include "preprocessor.h"

#include "lexer.h"
#include "log.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* weave_preprocessor_error_str(weave_preprocessor_error_t error) {
    switch (error) {
        case WEAVE_PREPROCESSOR_ERROR_NONE:
            return "No error";
        case WEAVE_PREPROCESSOR_ERROR_UNKNOWN_DIRECTIVE:
            return "Unknown directive";
        case WEAVE_PREPROCESSOR_ERROR_INVALID_MACRO_ARG_NAME:
            return "Invalid macro argument name";
        case WEAVE_PREPROCESSOR_ERROR_INVALID_MACRO_ARG_COUNT:
            return "Invalid macro argument count";
    }

    return "Unknown error";
}

void weave_macro_free(weave_macro_t* macro) {
    for (usize i = 0; i < macro->len_tokens; i++) {
        weave_token_free(&macro->tokens[i]);
    }
    for (usize i = 0; i < macro->len_args; i++) {
        free(macro->arg_names[i]);
    }
    free(macro->tokens);
    free(macro->arg_names);
    free(macro);
}

weave_preprocessor_t* weave_preprocessor_new(weave_lexer_t* lexer) {
    weave_preprocessor_t* preprocessor = malloc(sizeof(weave_preprocessor_t));
    preprocessor->lexer = lexer;
    preprocessor->at_eof = false;

    preprocessor->macro_names = malloc(sizeof(char*) * 8);
    preprocessor->macros = malloc(sizeof(weave_macro_t*) * 8);
    preprocessor->num_macros = 0;
    preprocessor->cap_macros = 8;

    preprocessor->current_macro = NULL;
    preprocessor->current_macro_arg_index = 0;
    preprocessor->current_macro_token_index = 0;

    preprocessor->macro_start_pos = (weave_token_pos_t){ 1, 1 };

    return preprocessor;
}

void weave_preprocessor_free(weave_preprocessor_t* preprocessor) {
    for (usize i = 0; i < preprocessor->num_macros; i++) {
        free(preprocessor->macro_names[i]);
        weave_macro_free(preprocessor->macros[i]);
    }
    free(preprocessor->macro_names);
    free(preprocessor->macros);
    weave_lexer_free(preprocessor->lexer);
    free(preprocessor);
}

void weave_preprocessor_advance(weave_preprocessor_t* preprocessor) {
    weave_lexer_result_t lexer_result = weave_lexer_next(preprocessor->lexer);
    if (!lexer_result.is_ok) {
        LOG_ERROR(
            "lexer error at %d:%d: %s",
            preprocessor->lexer->line,
            preprocessor->lexer->col,
            weave_lexer_error_str(lexer_result.err)
        );
        exit(1);
    }

    weave_token_t token = lexer_result.ok;

    preprocessor->current_lexer_token = token;

    if (token.ty == WEAVE_TOKEN_EOF) {
        preprocessor->at_eof = true;
        return;
    }
}

static void weave_preprocessor_register_macro(weave_preprocessor_t* preprocessor) {
    weave_lexer_result_t lexer_result = weave_lexer_next(preprocessor->lexer);

    if (!lexer_result.is_ok) {
        LOG_ERROR(
            "lexer error registering macro at %d:%d: %s",
            preprocessor->lexer->line,
            preprocessor->lexer->col,
            weave_lexer_error_str(lexer_result.err)
        );
        exit(1);
    }

    char* macro_name_raw = lexer_result.ok.val.str_val.val;
    u16 macro_name_len = lexer_result.ok.val.str_val.len;

    char* macro_name = malloc(sizeof(char) * (macro_name_len + 1));
    memcpy(macro_name, macro_name_raw, macro_name_len);
    macro_name[macro_name_len] = '\0';

    weave_token_free(&lexer_result.ok);

    for (usize i = 0; i < preprocessor->num_macros; i++) {
        if (strcmp(preprocessor->macro_names[i], macro_name) == 0) {
            LOG_ERROR(
                "macro redefinition at %d:%d",
                preprocessor->lexer->line,
                preprocessor->lexer->col
            );
            exit(1);
        }
    }

    if (preprocessor->num_macros == preprocessor->cap_macros) {
        preprocessor->cap_macros *= 2;
        preprocessor->macro_names =
            realloc(preprocessor->macro_names, sizeof(char*) * preprocessor->cap_macros);
        preprocessor->macros =
            realloc(preprocessor->macros, sizeof(weave_macro_t*) * preprocessor->cap_macros);
    }

    preprocessor->macro_names[preprocessor->num_macros] = macro_name;
    preprocessor->macros[preprocessor->num_macros] = malloc(sizeof(weave_macro_t));

    weave_macro_t* macro = preprocessor->macros[preprocessor->num_macros];

    macro->arg_names = malloc(sizeof(char*) * 8);
    macro->len_args = 0;
    macro->cap_args = 8;
    macro->tokens = malloc(sizeof(weave_token_t) * 8);
    macro->cap_tokens = 8;
    macro->len_tokens = 0;

    lexer_result = weave_lexer_next(preprocessor->lexer);

    if (!lexer_result.is_ok) {
        LOG_ERROR(
            "lexer error registering macro at %d:%d: %s\n",
            preprocessor->lexer->line,
            preprocessor->lexer->col,
            weave_lexer_error_str(lexer_result.err)
        );
        exit(1);
    }

    weave_token_t token = lexer_result.ok;

    while (token.ty == WEAVE_TOKEN_IDENTIFIER) {
        char* arg_name_raw = token.val.str_val.val;
        u16 arg_name_len = token.val.str_val.len;

        char* arg_name = malloc(sizeof(char) * (arg_name_len + 1));
        memcpy(arg_name, arg_name_raw, arg_name_len);
        arg_name[arg_name_len] = '\0';

        weave_token_free(&token);

        if (macro->len_args == macro->cap_args) {
            macro->cap_args *= 2;
            macro->arg_names = realloc(macro->arg_names, sizeof(char*) * macro->cap_args);
        }

        macro->arg_names[macro->len_args] = arg_name;
        macro->len_args++;

        lexer_result = weave_lexer_next(preprocessor->lexer);

        if (!lexer_result.is_ok) {
            LOG_ERROR(
                "lexer error registering macro at %d:%d: %s\n",
                preprocessor->lexer->line,
                preprocessor->lexer->col,
                weave_lexer_error_str(lexer_result.err)
            );
            exit(1);
        }

        token = lexer_result.ok;
    }

    if (token.ty != WEAVE_TOKEN_COLON) {
        LOG_ERROR(
            "expected ':' after macro arguments at %d:%d\n",
            preprocessor->lexer->line,
            preprocessor->lexer->col
        );
        exit(1);
    }

    lexer_result = weave_lexer_next(preprocessor->lexer);

    if (!lexer_result.is_ok) {
        LOG_ERROR(
            "lexer error registering macro at %d:%d: %s\n",
            preprocessor->lexer->line,
            preprocessor->lexer->col,
            weave_lexer_error_str(lexer_result.err)
        );
        exit(1);
    }

    token = lexer_result.ok;

    while (token.ty != WEAVE_TOKEN_SEMICOLON) {
        if (macro->len_tokens == macro->cap_tokens) {
            macro->cap_tokens *= 2;
            macro->tokens = realloc(macro->tokens, sizeof(weave_token_t) * macro->cap_tokens);
        }

        macro->tokens[macro->len_tokens] = token;
        macro->len_tokens++;

        lexer_result = weave_lexer_next(preprocessor->lexer);

        if (!lexer_result.is_ok) {
            LOG_ERROR(
                "lexer error registering macro at %d:%d: %s\n",
                preprocessor->lexer->line,
                preprocessor->lexer->col,
                weave_lexer_error_str(lexer_result.err)
            );
            exit(1);
        }

        token = lexer_result.ok;

        if (token.ty == WEAVE_TOKEN_EOF) {
            LOG_ERROR(
                "unexpected EOF while registering macro at %d:%d\n",
                preprocessor->lexer->line,
                preprocessor->lexer->col
            );
            exit(1);
        }
    }

    preprocessor->num_macros++;

    if (token.ty == WEAVE_TOKEN_SEMICOLON) {
        lexer_result = weave_lexer_next(preprocessor->lexer);

        if (!lexer_result.is_ok) {
            LOG_ERROR(
                "lexer error registering macro at %d:%d: %s\n",
                preprocessor->lexer->line,
                preprocessor->lexer->col,
                weave_lexer_error_str(lexer_result.err)
            );
            exit(1);
        }
    } else {
        LOG_ERROR(
            "expected ';' after macro definition at %d:%d\n",
            preprocessor->lexer->line,
            preprocessor->lexer->col
        );
        exit(1);
    }
}

static weave_token_t
weave_preprocessor_step_current_macro_invocation(weave_preprocessor_t* preprocessor) {
    weave_macro_t* macro = preprocessor->current_macro;

    if (preprocessor->in_macro_arg_expansion) {
        weave_token_t token_to_return = weave_token_clone(
            &preprocessor->current_macro_args[preprocessor->current_macro_arg_index]
                 .arg_tokens[preprocessor->current_macro_arg_expansion_index]
        );

        preprocessor->current_macro_arg_expansion_index++;

        if (preprocessor->current_macro_arg_expansion_index ==
            preprocessor->current_macro_args[preprocessor->current_macro_arg_index]
                .arg_token_len) {
            preprocessor->current_macro_arg_expansion_index = 0;
            preprocessor->current_macro_arg_index = 0;
            preprocessor->in_macro_arg_expansion = false;

            preprocessor->current_macro_token_index++;
        }

        return token_to_return;
    }

    if (preprocessor->current_macro_token_index == macro->len_tokens) {
        for (u32 i = 0; i < preprocessor->current_macro->len_args; i++) {
            for (u32 j = 0; j < preprocessor->current_macro_args[i].arg_token_len; j++) {
                weave_token_free(&preprocessor->current_macro_args[i].arg_tokens[j]);
            }
            free(preprocessor->current_macro_args[i].arg_tokens);
        }
        free(preprocessor->current_macro_args);

        preprocessor->current_macro = NULL;
        preprocessor->current_macro_token_index = 0;
        preprocessor->current_macro_arg_index = 0;
        preprocessor->current_macro_arg_expansion_index = 0;
        preprocessor->in_macro_arg_expansion = false;

        return preprocessor->current_lexer_token;
    }

    weave_token_t token =
        weave_token_clone(&macro->tokens[preprocessor->current_macro_token_index]);
    preprocessor->current_macro_token_index++;

    if (token.ty == WEAVE_TOKEN_DOLLAR) {
        token = macro->tokens[preprocessor->current_macro_token_index];

        if (token.ty != WEAVE_TOKEN_IDENTIFIER) {
            LOG_ERROR(
                "expected identifier after '$' in macro invocation at %d:%d\n",
                preprocessor->lexer->line,
                preprocessor->lexer->col
            );
            exit(1);
        }

        char* arg_name_raw = token.val.str_val.val;
        u16 arg_name_len = token.val.str_val.len;

        char* arg_name = malloc(sizeof(char) * (arg_name_len + 1));
        memcpy(arg_name, arg_name_raw, arg_name_len);
        arg_name[arg_name_len] = '\0';

        bool found_arg = false;
        usize arg_index = 0;

        for (usize i = 0; i < macro->len_args; i++) {
            if (strcmp(arg_name, macro->arg_names[i]) == 0) {
                found_arg = true;
                arg_index = i;
                break;
            }
        }

        if (!found_arg) {
            LOG_ERROR(
                "unknown macro argument '%s' in macro invocation at %d:%d\n",
                arg_name,
                preprocessor->lexer->line,
                preprocessor->lexer->col
            );
            exit(1);
        }

        free(arg_name);

        preprocessor->current_macro_arg_index = arg_index;
        preprocessor->current_macro_arg_expansion_index = 0;
        preprocessor->in_macro_arg_expansion = true;

        return weave_preprocessor_step_current_macro_invocation(preprocessor);
    }

    return token;
}

weave_token_t weave_preprocessor_next(weave_preprocessor_t* preprocessor) {
    if (preprocessor->current_macro != NULL) {
        return weave_preprocessor_step_current_macro_invocation(preprocessor);
    }

    weave_preprocessor_advance(preprocessor);

    weave_token_t token = preprocessor->current_lexer_token;

    if (token.ty == WEAVE_TOKEN_BANG) {
        weave_preprocessor_advance(preprocessor);

        token = preprocessor->current_lexer_token;

        switch (token.ty) {
            case WEAVE_TOKEN_MACRO:
                weave_preprocessor_register_macro(preprocessor);
                weave_preprocessor_advance(preprocessor);

                token = preprocessor->current_lexer_token;
                break;
            default:
                LOG_ERROR(
                    "unknown preprocessor directive '%s' at %d:%d\n",
                    token.val.str_val.val,
                    preprocessor->lexer->line,
                    preprocessor->lexer->col
                );
                exit(1);
        }
    }

    if (token.ty == WEAVE_TOKEN_IDENTIFIER) {
        weave_token_t token_to_free = token;
        for (usize i = 0; i < preprocessor->num_macros; i++) {
            if (strncmp(
                    preprocessor->macro_names[i],
                    token.val.str_val.val,
                    token.val.str_val.len
                ) == 0 &&
                preprocessor->macro_names[i][token.val.str_val.len] == '\0') {
                preprocessor->current_macro = preprocessor->macros[i];
                preprocessor->current_macro_token_index = 0;
                preprocessor->current_macro_arg_expansion_index = 0;
                preprocessor->in_macro_arg_expansion = false;

                preprocessor->current_macro_args = malloc(
                    sizeof(*preprocessor->current_macro_args) *
                    preprocessor->current_macro->len_args
                );

                weave_preprocessor_advance(preprocessor);

                token = preprocessor->current_lexer_token;

                // store macro invocation args
                for (usize j = 0; j < preprocessor->current_macro->len_args; j++) {
                    preprocessor->current_macro_args[j].arg_tokens =
                        malloc(sizeof(weave_token_t));
                    preprocessor->current_macro_args[j].arg_token_len = 0;
                    preprocessor->current_macro_args[j].arg_token_cap = 1;

                    while (token.ty != WEAVE_TOKEN_COMMA && token.ty != WEAVE_TOKEN_NEWLINE) {
                        if (preprocessor->current_macro_args[j].arg_token_len ==
                            preprocessor->current_macro_args[j].arg_token_cap) {
                            preprocessor->current_macro_args[j].arg_token_cap *= 2;
                            preprocessor->current_macro_args[j].arg_tokens = realloc(
                                preprocessor->current_macro_args[j].arg_tokens,
                                sizeof(weave_token_t) *
                                    preprocessor->current_macro_args[j].arg_token_cap
                            );
                        }

                        preprocessor->current_macro_args[j]
                            .arg_tokens[preprocessor->current_macro_args[j].arg_token_len] =
                            token;

                        preprocessor->current_macro_args[j].arg_token_len++;

                        weave_preprocessor_advance(preprocessor);

                        token = preprocessor->current_lexer_token;
                    }

                    if (token.ty == WEAVE_TOKEN_NEWLINE) {
                        if (j < preprocessor->current_macro->len_args - 1) {
                            LOG_ERROR(
                                "expected more arguments to macro '%s' at %d:%d\n",
                                preprocessor->macro_names[i],
                                preprocessor->lexer->line,
                                preprocessor->lexer->col
                            );
                            exit(1);
                        } else {
                            break;
                        }
                    } else if (token.ty == WEAVE_TOKEN_COMMA) {
                        weave_preprocessor_advance(preprocessor);

                        token = preprocessor->current_lexer_token;
                    }
                }

                weave_token_free(&token_to_free);
                return weave_preprocessor_step_current_macro_invocation(preprocessor);
            }
        }
    }

    return token;
}
