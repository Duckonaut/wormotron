#include "log.h"
#include "types.h"
#include "wormotron.h"
#include "rom.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RELEASE
#define WT_LOG_LEVEL SDL_LOG_PRIORITY_INFO
#else
#define WT_LOG_LEVEL SDL_LOG_PRIORITY_DEBUG
#endif

typedef struct args {
    char* rom_file;
} args_t;

static void usage(void) {
    printf("Usage: wormotron <rom_file>\n");
}

static args_t parse_args(int argc, char* argv[]) {
    args_t args;
    if (argc < 2 || argc > 5) {
        usage();
        exit(1);
    }

    args.rom_file = argv[1];

    return args;
}

void terminate_handler(int signal) {
    (void)signal; // unused
    g_stop = true;
}

// TODO: This is a hack to get around SDL's main macro.
#undef main
int main(int argc, char* argv[]) {
    log_init();

    signal(SIGINT, terminate_handler);
    signal(SIGTERM, terminate_handler);

    args_t args = parse_args(argc, argv);

    wormotron_t* wormotron = wormotron_new(args.rom_file);
    g_wormotron = wormotron;

    wormotron_run(wormotron);

    // Cleanup.
    wormotron_free(wormotron);

    log_close();
    return 0;
}
