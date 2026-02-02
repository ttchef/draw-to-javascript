
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <raylib.h>

#define UNDO_COUNT 10
#define UI_MAX_INPUT_CHARACTERS 20

enum uiMode {
    UI_MODE_FILE_SELECTION,
    UI_MODE_IMAGE_EDITING,
    UI_MODE_EXPORT,
};

typedef struct Vector2I {
    int32_t x;
    int32_t y;
} Vector2I;

typedef struct PixelState {
    int32_t index;
    float radius;
    Color old_color;
    Color new_color;
} PixelState;

typedef struct uiState {
    bool new_image_menu;
    bool image_menu_width_input;
    bool image_menu_height_input;
    int32_t image_menu_width_index;
    int32_t image_menu_height_index;
    char image_width[UI_MAX_INPUT_CHARACTERS];
    char image_height[UI_MAX_INPUT_CHARACTERS];
} uiState;

typedef struct Context {
    int32_t window_width;
    int32_t window_height;
    int32_t new_image_width;
    int32_t new_image_height;
    uint8_t* image_data;
    Texture2D loaded_tex;
    float loaded_ratio;
    enum uiMode mode;
    Color clear_color;
    Color draw_color;
    Color ignore_color;
    float brush_size;
    bool export_x_mirrored;
    bool export_one_line;
    bool pick_color_draw;
    bool pick_color_ignore;
    bool draw_ignored_pixels;
    bool debug_mode;
    bool drawing;
    Vector2 previous_mouse_pos;
    Vector2 current_mouse_pos;
    Camera2D camera;
    float export_scale;
    PixelState* save_states[UNDO_COUNT];
    int32_t save_states_index;
    uiState ui_state;
} Context;

void update_ui(struct Context* ctx);
void compute_clay_layout(struct Context* ctx, Texture2D* textures, size_t image_count);
void draw_ui(struct Context* ctx, Font* fonts);

#endif // COMMON_H
