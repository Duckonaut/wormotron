#include <stdio.h>
#include "SDL.h"
#include "SDL_log.h"
#include "SDL_video.h"
#include "squirm.h"

#define WT_WINDOW_TITLE "dev: wormotron"
#define WT_WINDOW_LOGICAL_WIDTH 144
#define WT_WINDOW_LOGICAL_HEIGHT 96
#define WT_WINDOW_SCALE 4
#define WT_WINDOW_WIDTH (WT_WINDOW_LOGICAL_WIDTH * WT_WINDOW_SCALE)
#define WT_WINDOW_HEIGHT (WT_WINDOW_LOGICAL_HEIGHT * WT_WINDOW_SCALE)

#ifdef RELEASE
#define WT_LOG_LEVEL SDL_LOG_PRIORITY_INFO
#else
#define WT_LOG_LEVEL SDL_LOG_PRIORITY_DEBUG
#endif

int main(int argc, char* argv[]) {
    (void)argc; // unused
    (void)argv; // unused

    // Set logging parameters.
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, WT_LOG_LEVEL);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Starting wormotron...\n");

    // Initialize SDL. We will be using a software renderer.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "SDL initialization failed: %s\n",
            SDL_GetError()
        );
        return 1;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized.\n");

    // Create a window.
    SDL_Window* window = SDL_CreateWindow(
        WT_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WT_WINDOW_WIDTH,
        WT_WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create window: %s\n",
            SDL_GetError()
        );
        return 1;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Window created.\n");

    // Create a renderer.
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if (renderer == NULL) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create renderer: %s\n",
            SDL_GetError()
        );
        return 1;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer created.\n");

    // Set the logical size of the renderer.
    SDL_RenderSetLogicalSize(renderer, WT_WINDOW_LOGICAL_WIDTH, WT_WINDOW_LOGICAL_HEIGHT);

    // Main loop.

    bool running = true;

    while (running) {
        // Update the game state.
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Render the game state.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_Rect rect = { .x = WT_WINDOW_LOGICAL_WIDTH / 2 - 4,
                          .y = WT_WINDOW_LOGICAL_HEIGHT / 2 - 4,
                          .w = 8,
                          .h = 8 };

        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting wormotron...\n");

    // Cleanup.
    SDL_Quit();

    return 0;
}
