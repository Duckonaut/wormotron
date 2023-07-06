#pragma once

#include "SDL_mutex.h"
#include "types.h"

#include "SDL.h"

#define WT_WINDOW_TITLE "dev: wormotron"

#define WT_WINDOW_LOGICAL_WIDTH 144
#define WT_WINDOW_LOGICAL_HEIGHT 96

#define WT_WINDOW_SCALE 4

#define WT_WINDOW_WIDTH (WT_WINDOW_LOGICAL_WIDTH * WT_WINDOW_SCALE)
#define WT_WINDOW_HEIGHT (WT_WINDOW_LOGICAL_HEIGHT * WT_WINDOW_SCALE)

#define WT_GRAPHICS_RAM_SIZE 0x4000
#define WT_GRAPHICS_RAM_START 0x8000

#define WT_GRAPHICS_PALETTE_SIZE 16
#define WT_GRAPHICS_PALETTE_START WT_GRAPHICS_RAM_SIZE - (16 * 4)
#define WT_GRAPHICS_PALETTE_END WT_GRAPHICS_RAM_SIZE

#define WT_GRAPHICS_MAX_COMMANDS 4096

typedef enum wormotron_graphics_mode {
    WT_GRAPHICS_MODE_RAW,
    WT_GRAPHICS_MODE_TEXT,
    WT_GRAPHICS_MODE_TILED,
    WT_GRAPHICS_MODE_SPRITEONLY,
} wormotron_graphics_mode_t;

typedef struct wormotron_graphics_command {
    u16 address;
    u8 value;
} wormotron_graphics_command_t;

typedef struct wormotron_graphics {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    wormotron_graphics_mode_t mode;

    SDL_Color palette[WT_GRAPHICS_PALETTE_SIZE];

    SDL_mutex* mutex;

    u8 ram[WT_GRAPHICS_RAM_SIZE];
} wormotron_graphics_t;

wormotron_graphics_t* wormotron_graphics_new(void);
void wormotron_graphics_free(wormotron_graphics_t* graphics);

void wormotron_graphics_clear(wormotron_graphics_t* graphics);
void wormotron_graphics_flush(wormotron_graphics_t* graphics);
void wormotron_graphics_present(wormotron_graphics_t* graphics);

void wormotron_graphics_write(wormotron_graphics_t* graphics, u16 address, u8 value);
u8 wormotron_graphics_read(wormotron_graphics_t* graphics, u16 address);
// void wormotron_graphics_set_mode(
//     wormotron_graphics_t* graphics,
//     wormotron_graphics_mode_t mode
// );
void wormotron_graphics_draw_pixel(wormotron_graphics_t* graphics, u8 x, u8 y, u8 color);
// void wormotron_graphics_draw_sprite(wormotron_graphics_t* graphics, u8 x, u8 y, u8* sprite);
// void wormotron_graphics_draw_sprite_flip_x(
//     wormotron_graphics_t* graphics,
//     u8 x,
//     u8 y,
//     u8* sprite
// );
// void wormotron_graphics_draw_sprite_flip_y(
//     wormotron_graphics_t* graphics,
//     u8 x,
//     u8 y,
//     u8* sprite
// );
//
// void wormotron_graphics_draw_sprite_flip_xy(
//     wormotron_graphics_t* graphics,
//     u8 x,
//     u8 y,
//     u8* sprite
// );
// void wormotron_graphics_set_palette(wormotron_graphics_t* graphics, SDL_Color* palette);
// void wormotron_graphics_set_palette_color_rgb(
//     wormotron_graphics_t* graphics,
//     u8 index,
//     u8 r,
//     u8 g,
//     u8 b
// );
// void wormotron_graphics_set_tile(
//     wormotron_graphics_t* graphics,
//     u8 tilemap,
//     u8 x,
//     u8 y,
//     u8 tile_data[64]
// );
// void wormotron_graphics_put_tile(wormotron_graphics_t* graphics, u8 layer, u8 x, u8 y, u8
// tile); void wormotron_graphics_set_tilemap(wormotron_graphics_t* graphics, u8 layer, u8
// tilemap);
