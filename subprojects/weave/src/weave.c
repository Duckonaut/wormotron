#include "weave.h"
#include "burrow.h"
#include "preprocessor.h"
#include "weave.h"
#include "lexer.h"
#include "log.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void weave_advance(weave_t* weave);
static bool weave_match_token_type(weave_t* weave, weave_token_ty_t ty);
static void weave_consume_token_type(weave_t* weave, weave_token_ty_t ty);

static weave_t* weave_new(weave_preprocessor_t* preprocessor, FILE* output) {
    weave_t* weave = malloc(sizeof(weave_t));
    weave->preprocessor = preprocessor;
    weave->output = output;

    weave->num_labels = 0;
    weave->addr = 0;
    weave->at_eof = false;
    weave->token.ty = WEAVE_TOKEN_INVALID;

    weave_advance(weave);

    return weave;
}

static void weave_free(weave_t* weave) {
    for (size_t i = 0; i < weave->num_labels; i++) {
        free(weave->labels[i].name);
        if (weave->labels[i].unresolved_refs) {
            free(weave->labels[i].unresolved_refs);
        }
    }
    weave_token_free(&weave->token);
    weave_preprocessor_free(weave->preprocessor);
    free(weave);
}

static void weave_emit_instruction(weave_t* weave, burrow_op_t op) {
    fputc(op.op, weave->output);
    weave->addr++;
    fputc(op.regs.dest, weave->output);
    weave->addr++;
    fputc(op.regs.src_a, weave->output);
    weave->addr++;
    fputc(op.regs.src_b, weave->output);
    weave->addr++;
}

static void weave_register_label(weave_t* weave, const weave_token_t* label) {
    if (weave->num_labels == WEAVE_MAX_LABELS) {
        LOG_ERROR(
            "internal error at %d:%d: Too many labels\n",
            label->pos.line,
            label->pos.col
        );
        exit(1);
    }

    for (size_t i = 0; i < weave->num_labels; i++) {
        if (strlen(weave->labels[i].name) == label->val.str_val.len &&
            memcmp(weave->labels[i].name, label->val.str_val.val, label->val.str_val.len) ==
                0) {
            if (weave->labels[i].defined) {
                LOG_ERROR(
                    "internal error at %d:%d: Label '%.*s' already defined\n",
                    label->pos.line,
                    label->pos.col,
                    label->val.str_val.len,
                    label->val.str_val.val
                );
                exit(1);
            } else {
                weave->labels[i].defined = true;
                weave->labels[i].addr = weave->addr;
            }
        }
    }

    weave_label_t* new_label = &weave->labels[weave->num_labels++];

    new_label->defined = true;
    new_label->unresolved_refs = NULL;
    new_label->unresolved_refs_len = 0;
    new_label->unresolved_refs_cap = 0;

    new_label->name = malloc(label->val.str_val.len + 1);
    memcpy(new_label->name, label->val.str_val.val, label->val.str_val.len);
    new_label->name[label->val.str_val.len] = '\0';

    new_label->addr = weave->addr;
}

static u16
weave_get_label_value(weave_t* weave, const char* label_name, size_t label_name_len) {
    for (size_t i = 0; i < weave->num_labels; i++) {
        if (strlen(weave->labels[i].name) == label_name_len &&
            memcmp(weave->labels[i].name, label_name, label_name_len) == 0) {
            // check if label is defined
            if (!weave->labels[i].defined) {
                // add to unresolved_refs
                if (weave->labels[i].unresolved_refs_len ==
                    weave->labels[i].unresolved_refs_cap) {
                    weave->labels[i].unresolved_refs_cap =
                        weave->labels[i].unresolved_refs_cap == 0
                            ? 1
                            : weave->labels[i].unresolved_refs_cap * 2;
                    weave->labels[i].unresolved_refs = realloc(
                        weave->labels[i].unresolved_refs,
                        sizeof(u16) * weave->labels[i].unresolved_refs_cap
                    );
                }

                weave->labels[i].unresolved_refs[weave->labels[i].unresolved_refs_len++] =
                    weave->addr;

                return 0;
            } else {
                return weave->labels[i].addr;
            }
        }
    }

    // not found, register new label as unresolved
    if (weave->num_labels == WEAVE_MAX_LABELS) {
        LOG_ERROR("internal error: Too many labels\n");
        exit(1);
    }

    weave_label_t* new_label = &weave->labels[weave->num_labels++];

    new_label->defined = false;
    new_label->unresolved_refs = malloc(sizeof(u16) * 4);
    new_label->unresolved_refs_len = 1;
    new_label->unresolved_refs_cap = 4;

    new_label->name = malloc(label_name_len + 1);
    memcpy(new_label->name, label_name, label_name_len);
    new_label->name[label_name_len] = '\0';

    new_label->unresolved_refs[0] = weave->addr + 2; // labels will always be in immediate

    return 0;
}

typedef enum burrow_instruction_arg_scheme {
    BURROW_INSTRUCTION_ARG_SCHEME_NONE,    // eg. nop
    BURROW_INSTRUCTION_ARG_SCHEME_REGS,    // eg. add %a, %b, %c
    BURROW_INSTRUCTION_ARG_SCHEME_REG_IMM, // eg. ldi %a, 0x1234
    BURROW_INSTRUCTION_ARG_SCHEME_REG_REG, // eg. str %a, %b
    BURROW_INSTRUCTION_ARG_SCHEME_REG,     // eg. jd %a
    BURROW_INSTRUCTION_ARG_SCHEME_IMM,     // eg. jmp 0x1234
} burrow_instruction_arg_scheme_t;

typedef struct burrow_instruction_def {
    const char* name;
    burrow_instruction_arg_scheme_t arg_scheme;
} burrow_instruction_def_t;

static const burrow_instruction_def_t k_instruction_defs[BURROW_OP_COUNT] = {
    [BURROW_OP_NOP] = {
        .name = "nop",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_NONE,
    },
    [BURROW_OP_LDI] = {
        .name = "ldi",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_IMM,
    },
    [BURROW_OP_LDR] = {
        .name = "ldr",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_REG,
    },
    [BURROW_OP_LDRB] = {
        .name = "ldrb",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_REG,
    },
    [BURROW_OP_ADD] = {
        .name = "add",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_SUB] = {
        .name = "sub",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_MUL] = {
        .name = "mul",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_DIV] = {
        .name = "div",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_MOD] = {
        .name = "mod",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_AND] = {
        .name = "and",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_OR] = {
        .name = "or",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_XOR] = {
        .name = "xor",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_SHL] = {
        .name = "shl",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_SHR] = {
        .name = "shr",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REGS,
    },
    [BURROW_OP_JMP] = {
        .name = "jmp",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_IMM,
    },
    [BURROW_OP_JZ] = {
        .name = "jz",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_IMM,
    },
    [BURROW_OP_JD] = {
        .name = "jd",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG,
    },
    [BURROW_OP_STI] = {
        .name = "sti",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_IMM,
    },
    [BURROW_OP_STIB] = {
        .name = "stib",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_IMM,
    },
    [BURROW_OP_STR] = {
        .name = "str",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_REG,
    },
    [BURROW_OP_STRB] = {
        .name = "strb",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_REG_REG,
    },
    [BURROW_OP_SYS] = {
        .name = "sys",
        .arg_scheme = BURROW_INSTRUCTION_ARG_SCHEME_NONE,
    },
};

static u8 weave_parse_register(weave_t* weave) {
    weave_consume_token_type(weave, WEAVE_TOKEN_PERCENT);

    if (weave->token.ty != WEAVE_TOKEN_IDENTIFIER) {
        LOG_ERROR(
            "parser error at %d:%d: Expected identifier after %%\n",
            weave->token.pos.line,
            weave->token.pos.col
        );
        exit(1);
    }

    u8 reg =
        burrow_register_from_str(weave->token.val.str_val.val, weave->token.val.str_val.len);

    if (reg == BURROW_REG_INVALID) {
        LOG_ERROR(
            "parser error at %d:%d: Invalid register %%%.*s\n",
            weave->token.pos.line,
            weave->token.pos.col,
            (int)weave->token.val.str_val.len,
            weave->token.val.str_val.val
        );
        exit(1);
    }

    weave_advance(weave);

    return reg;
}

static u16 weave_parse_immediate(weave_t* weave) {
    switch (weave->token.ty) {
        case WEAVE_TOKEN_INT: {
            u16 num = weave->token.val.int_val;
            weave_advance(weave);
            return num;
        }
        case WEAVE_TOKEN_CHAR: {
            u16 num = weave->token.val.char_val;
            weave_advance(weave);
            return num;
        }
        case WEAVE_TOKEN_DOT: {
            weave_advance(weave);
            if (weave->token.ty != WEAVE_TOKEN_IDENTIFIER) {
                LOG_ERROR(
                    "parser error at %d:%d: Expected identifier after .\n",
                    weave->token.pos.line,
                    weave->token.pos.col
                );
                exit(1);
            }

            u16 addr = weave_get_label_value(
                weave,
                weave->token.val.str_val.val,
                weave->token.val.str_val.len
            );

            weave_advance(weave);

            return addr;
        }
        default:
            break;
    }

    LOG_ERROR(
        "parser error at %d:%d: Expected immediate\n",
        weave->token.pos.line,
        weave->token.pos.col
    );
    exit(1);
}

static u8 weave_op_from_str(const char* str, size_t len) {
    for (u8 i = 0; i < BURROW_OP_COUNT; i++) {
        if (strncmp(str, k_instruction_defs[i].name, len) == 0) {
            return i;
        }
    }

    return BURROW_OP_INVALID;
}

static void weave_parse_instruction(weave_t* weave) {
    burrow_op_t op = { 0 };

    if (weave->token.ty != WEAVE_TOKEN_IDENTIFIER) {
        LOG_ERROR(
            "parser error at %d:%d: Expected identifier\n",
            weave->token.pos.line,
            weave->token.pos.col
        );
        exit(1);
    }

    op.op = weave_op_from_str(weave->token.val.str_val.val, weave->token.val.str_val.len);

    if (op.op == BURROW_OP_INVALID) {
        LOG_ERROR(
            "parser error at %d:%d: Invalid instruction %.*s\n",
            weave->token.pos.line,
            weave->token.pos.col,
            (int)weave->token.val.str_val.len,
            weave->token.val.str_val.val
        );
        exit(1);
    }

    weave_advance(weave);

    burrow_instruction_arg_scheme_t arg_scheme = k_instruction_defs[op.op].arg_scheme;

    switch (arg_scheme) {
        case BURROW_INSTRUCTION_ARG_SCHEME_NONE:
            break;
        case BURROW_INSTRUCTION_ARG_SCHEME_REGS:
            op.regs.dest = weave_parse_register(weave);
            weave_consume_token_type(weave, WEAVE_TOKEN_COMMA);
            op.regs.src_a = weave_parse_register(weave);
            weave_consume_token_type(weave, WEAVE_TOKEN_COMMA);
            op.regs.src_b = weave_parse_register(weave);
            break;
        case BURROW_INSTRUCTION_ARG_SCHEME_REG_IMM:
            op.regs.dest = weave_parse_register(weave);
            weave_consume_token_type(weave, WEAVE_TOKEN_COMMA);
            u16 imm = weave_parse_immediate(weave);
            // store high byte in src_a and low byte in src_b
            op.regs.src_a = (imm >> 8) & 0xFF;
            op.regs.src_b = imm & 0xFF;
            break;
        case BURROW_INSTRUCTION_ARG_SCHEME_REG_REG:
            op.regs.dest = weave_parse_register(weave);
            weave_consume_token_type(weave, WEAVE_TOKEN_COMMA);
            op.regs.src_a = weave_parse_register(weave);
            op.regs.src_b = 0;
            break;
        case BURROW_INSTRUCTION_ARG_SCHEME_REG:
            op.regs.dest = weave_parse_register(weave);
            break;
        case BURROW_INSTRUCTION_ARG_SCHEME_IMM: {
            u16 imm = weave_parse_immediate(weave);
            // store high byte in src_a and low byte in src_b
            op.regs.src_a = (imm >> 8) & 0xFF;
            op.regs.src_b = imm & 0xFF;
            break;
        }
    }

    weave_consume_token_type(weave, WEAVE_TOKEN_NEWLINE);

    weave_emit_instruction(weave, op);
}

#define TOKEN_TYPE_CASE_PRINT_TYPE_SIMPLE(ty)                                                  \
    case WEAVE_TOKEN_##ty:                                                                     \
        fprintf(stdout, #ty "\n");                                                             \
        break;

static void weave_run(weave_t* weave) {
    while (!weave->at_eof) {
        while (weave->token.ty == WEAVE_TOKEN_NEWLINE) {
            weave_advance(weave);
        }

        if (weave_match_token_type(weave, WEAVE_TOKEN_DOT)) {
            if (weave->token.ty == WEAVE_TOKEN_IDENTIFIER) {
                weave_register_label(weave, &weave->token);
                weave_advance(weave);
            } else {
                LOG_ERROR(
                    "parser error at %d:%d: Expected identifier after dot\n",
                    weave->token.pos.line,
                    weave->token.pos.col
                );
                exit(1);
            }
            weave_consume_token_type(weave, WEAVE_TOKEN_COLON);
        } else if (weave->token.ty == WEAVE_TOKEN_IDENTIFIER) {
            // instruction
            weave_parse_instruction(weave);
        } else {
            LOG_ERROR(
                "parser error at %d:%d: Expected instruction or label, got %s\n",
                weave->token.pos.line,
                weave->token.pos.col,
                weave_token_ty_str(weave->token.ty)
            );
            exit(1);
        }
    }

    // backpatch labels
    for (size_t i = 0; i < weave->num_labels; i++) {
        weave_label_t* label = &weave->labels[i];
        if (!label->defined) {
            LOG_ERROR("error: Label %s not found\n", label->name);
        } else {
            for (size_t j = 0; j < label->unresolved_refs_len; j++) {
                u16 addr = label->unresolved_refs[j];
                usize addr_full = (usize)addr;

                fseek(weave->output, addr_full, SEEK_SET);
                u8 low_byte = label->addr & 0xFF;
                u8 high_byte = (label->addr >> 8) & 0xFF;
                // write high byte first
                fwrite(&high_byte, 1, 1, weave->output);
                fwrite(&low_byte, 1, 1, weave->output);
            }
        }
    }
}

void weave_process(FILE* input, FILE* output) {
    weave_lexer_t* lexer = weave_lexer_new(input);

    weave_preprocessor_t* preprocessor = weave_preprocessor_new(lexer);

    weave_t* weave = weave_new(preprocessor, output);

    weave_run(weave);

    weave_free(weave);
}

static void weave_print_token(weave_token_t token) {
    switch (token.ty) {
        case WEAVE_TOKEN_INVALID:
            fprintf(stderr, "Invalid token\n");
            break;
        case WEAVE_TOKEN_EOF:
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
            printf("IDENTIFIER: ");
            fwrite(token.val.str_val.val, 1, token.val.str_val.len, stdout);
            printf("\n");
            break;
        case WEAVE_TOKEN_INT:
            printf("INT: %d\n", token.val.int_val);
            break;
        case WEAVE_TOKEN_STR:
            printf("STR: \"");
            fwrite(token.val.str_val.val, 1, token.val.str_val.len, stdout);
            printf("\"\n");
            break;
        case WEAVE_TOKEN_CHAR:
            printf("CHAR: '%c'\n", token.val.char_val);
            break;
    }
}

static void weave_advance(weave_t* weave) {
    if (weave->token.ty != WEAVE_TOKEN_INVALID) {
        weave_token_free(&weave->token);
    }

    weave->token = weave_preprocessor_next(weave->preprocessor);

    // weave_print_token(weave->token);

    weave->at_eof = weave->token.ty == WEAVE_TOKEN_EOF;
}

static void weave_consume_token_type(weave_t* weave, weave_token_ty_t ty) {
    if (weave->token.ty != ty) {
        LOG_ERROR(
            "lexer error at %d:%d: Expected token type %s, got %s\n",
            weave->token.pos.line,
            weave->token.pos.col,
            weave_token_ty_str(ty),
            weave_token_ty_str(weave->token.ty)
        );
        exit(1);
    }

    weave_advance(weave);
}

static bool weave_match_token_type(weave_t* weave, weave_token_ty_t ty) {
    if (weave->token.ty == ty) {
        weave_advance(weave);
        return true;
    }

    return false;
}
