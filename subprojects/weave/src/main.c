#include "log.h"
#include "stdio.h"
#include "types.h"
#include "weave.h"
#include <stdlib.h>
#include <string.h>

typedef struct args {
    char* input_file;
    char* output_file;
} args_t;

static void usage(void) {
    printf("Usage: weave <input_file> [-o <output_file>]\n");
}

static void parse_args(int argc, char* argv[], args_t* args) {
    if (argc < 2) {
        usage();
        exit(1);
    }

    args->input_file = argv[1];
    args->output_file = NULL;

    bool output_file_specified = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (output_file_specified) {
                usage();
                exit(1);
            }

            if (i + 1 >= argc) {
                usage();
                exit(1);
            }
            args->output_file = argv[i + 1];
            output_file_specified = true;
            i++;
        } else {
            usage();
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    // init logging

    log_init();

    args_t args;
    parse_args(argc, argv, &args);

    FILE* input_file;

    if (args.input_file == NULL || strcmp(args.input_file, "-") == 0) {
        input_file = stdin;
    } else {
        input_file = fopen(args.input_file, "r");

        if (input_file == NULL) {
            LOG_ERROR("Failed to open input file: %s\n", args.input_file);
            exit(1);
        }
    }

    FILE* output_file;

    if (args.output_file == NULL || strcmp(args.output_file, "-") == 0) {
        output_file = stdout;
    } else {
        output_file = fopen(args.output_file, "w");

        if (output_file == NULL) {
            LOG_ERROR("Failed to open output file: %s\n", args.output_file);
            exit(1);
        }
    }

    weave_process(input_file, output_file);

    fclose(input_file);
    fclose(output_file);

    log_close();

    return 0;
}
