#include "graphics.h"

#include "SDL_mutex.h"
#include "burrow.h"
#include "log.h"

#include "SDL_pixels.h"
#include "SDL.h"
#include "SDL_render.h"
#include "SDL_hints.h"
#include "SDL_thread.h"
#include "SDL_video.h"
#include "types.h"
#include "wormotron.h"
#include <assert.h>

static const SDL_Color k_default_palette[16] = {
    { .r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF },
    { .r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF },
    { .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF },
    { .r = 0x00, .g = 0xFF, .b = 0xFF, .a = 0xFF },
    { .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF },
    { .r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0xFF },
    { .r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0xFF },
    { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF },
    { .r = 0x3f, .g = 0x3f, .b = 0x3f, .a = 0xFF },
    { .r = 0x00, .g = 0x00, .b = 0x7f, .a = 0xFF },
    { .r = 0x00, .g = 0x7f, .b = 0x00, .a = 0xFF },
    { .r = 0x00, .g = 0x7f, .b = 0x7f, .a = 0xFF },
    { .r = 0x7f, .g = 0x00, .b = 0x00, .a = 0xFF },
    { .r = 0x7f, .g = 0x00, .b = 0x7f, .a = 0xFF },
    { .r = 0x7f, .g = 0x7f, .b = 0x00, .a = 0xFF },
    { .r = 0xbf, .g = 0xbf, .b = 0xbf, .a = 0xFF },
};

wormotron_graphics_t* wormotron_graphics_new(void) {
    wormotron_graphics_t* graphics = malloc(sizeof(wormotron_graphics_t));

    if (graphics == NULL) {
        LOG_ERROR("Failed to allocate memory for graphics\n");
        exit(1);
    }

    // Initialize SDL. We will be using a software renderer.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    LOG_DEBUG("SDL initialized.\n");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    // Enable VSync.
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // Create a window.
    graphics->window = SDL_CreateWindow(
        WT_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WT_WINDOW_WIDTH,
        WT_WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (graphics->window == NULL) {
        LOG_ERROR("Failed to create window: %s\n", SDL_GetError());
        exit(1);
    }

    // Restrict the window size.
    SDL_SetWindowMinimumSize(graphics->window, WT_WINDOW_WIDTH, WT_WINDOW_HEIGHT);
    SDL_SetWindowMaximumSize(graphics->window, WT_WINDOW_WIDTH, WT_WINDOW_HEIGHT);
    SDL_SetWindowResizable(graphics->window, SDL_FALSE);

    LOG_DEBUG("Window created.\n");

    // Create a renderer.
    graphics->renderer = SDL_CreateRenderer(graphics->window, -1, SDL_RENDERER_SOFTWARE);

    if (graphics->renderer == NULL) {
        LOG_ERROR("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    LOG_DEBUG("Renderer created.\n");

    // Set the logical size of the renderer.
    SDL_RenderSetLogicalSize(
        graphics->renderer,
        WT_WINDOW_LOGICAL_WIDTH,
        WT_WINDOW_LOGICAL_HEIGHT
    );

    for (int i = 0; i < 16; i++) {
        graphics->palette[i] = k_default_palette[i];
    }

    // Create a texture.
    graphics->texture = SDL_CreateTexture(
        graphics->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        WT_WINDOW_LOGICAL_WIDTH,
        WT_WINDOW_LOGICAL_HEIGHT
    );

    if (graphics->texture == NULL) {
        LOG_ERROR("Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    LOG_DEBUG("Texture created.\n");

    graphics->mutex = SDL_CreateMutex();

    LOG_DEBUG("Graphics initialized.\n");

    return graphics;
}

void wormotron_graphics_free(wormotron_graphics_t* graphics) {
    SDL_DestroyTexture(graphics->texture);
    SDL_DestroyRenderer(graphics->renderer);
    SDL_DestroyWindow(graphics->window);
    SDL_DestroyMutex(graphics->mutex);
    SDL_Quit();
    free(graphics);
}

void wormotron_graphics_write(wormotron_graphics_t* graphics, u16 address, u8 value) {
    assert(address < WT_GRAPHICS_RAM_SIZE);

    graphics->ram[address] = value;
}

void wormotron_graphics_clear(wormotron_graphics_t* graphics) {
    SDL_SetRenderDrawColor(graphics->renderer, 0, 0, 0, 255);
    SDL_RenderClear(graphics->renderer);
}

void wormotron_graphics_flush(wormotron_graphics_t* graphics) {
    for (u16 i = 0; i < 16; i++) {
        SDL_Color color;
        color.r = graphics->ram[WT_GRAPHICS_PALETTE_START + i * 4 + 0];
        color.g = graphics->ram[WT_GRAPHICS_PALETTE_START + i * 4 + 1];
        color.b = graphics->ram[WT_GRAPHICS_PALETTE_START + i * 4 + 2];
        color.a = graphics->ram[WT_GRAPHICS_PALETTE_START + i * 4 + 3];

        graphics->palette[i] = color;
    }

    SDL_Color* pixels = NULL;
    int pitch = 0;

    SDL_LockTexture(graphics->texture, NULL, (void**)&pixels, &pitch);

    for (u16 y = 0; y < WT_WINDOW_LOGICAL_HEIGHT; y++) {
        for (u16 x = 0; x < WT_WINDOW_LOGICAL_WIDTH / 2; x++) {
            u8 colors = graphics->ram[y * WT_WINDOW_LOGICAL_WIDTH / 2 + x];
            u8 first = colors >> 4;
            u8 second = colors & 0x0F;

            pixels[y * WT_WINDOW_LOGICAL_WIDTH + x * 2] = graphics->palette[first];
            pixels[y * WT_WINDOW_LOGICAL_WIDTH + x * 2 + 1] = graphics->palette[second];
        }
    }

    SDL_UnlockTexture(graphics->texture);
}

void wormotron_graphics_present(wormotron_graphics_t* graphics) {
    SDL_RenderCopy(graphics->renderer, graphics->texture, NULL, NULL);

    SDL_RenderPresent(graphics->renderer);
}

void wormotron_graphics_draw_pixel(wormotron_graphics_t* graphics, u8 x, u8 y, u8 color) {
    assert(x < WT_WINDOW_LOGICAL_WIDTH);
    assert(y < WT_WINDOW_LOGICAL_HEIGHT);
    assert(color < WT_GRAPHICS_PALETTE_SIZE);

    SDL_Color* pixels = NULL;
    int pitch = 0;

    SDL_LockTexture(graphics->texture, NULL, (void**)&pixels, &pitch);

    pixels[y * WT_WINDOW_LOGICAL_WIDTH + x] = graphics->palette[color];

    SDL_UnlockTexture(graphics->texture);
}
