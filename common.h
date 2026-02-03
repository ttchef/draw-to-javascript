
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include <raylib.h>

#define UNDO_COUNT 10
#define UI_MAX_INPUT_CHARACTERS 4
#define BRUSH_COLORS_COUNT 2

#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof((arr)[0]))

enum uiMode {
    UI_MODE_FILE_SELECTION,
    UI_MODE_IMAGE_EDITING,
    UI_MODE_EXPORT,
};

enum uiTool {
    UI_TOOL_BRUSH,
    UI_TOOL_ERASER,
    UI_TOOL_BUCKET_FILL,
    // TODO: UI_TOOL_COLOR_PICKER (maybe)
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
    float top_bar_lerp;
    int32_t current_tool;

    /* New Image Menu */
    bool new_image_menu;
    bool image_menu_floating;
    Vector2I image_menu_pos;
    bool image_menu_width_input;
    bool image_menu_height_input;
    int32_t image_menu_width_index;
    int32_t image_menu_height_index;
    char image_width[UI_MAX_INPUT_CHARACTERS + 1]; /* NULL terminator */
    char image_height[UI_MAX_INPUT_CHARACTERS + 1];

    /* Color Picker Menu */
    bool color_picker_menu;
    bool color_picker_menu_floating;
    Vector2I color_picker_menu_pos;
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
    Color brush_colors[BRUSH_COLORS_COUNT];
    int32_t current_brush;
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
    Texture2D rainbow_circle;
} Context;

void image_to_javascript(Context* ctx, FILE* fd, char* name_x, char* name_y);
void load_from_javascript(Context* ctx);

void update_ui(struct Context* ctx);
void compute_clay_layout(struct Context* ctx, Texture2D* textures, size_t image_count);
void draw_ui(struct Context* ctx, Font* fonts);

static inline float lerp(float a, float b, float t) {
    return b * t + a * (1.0f - t);
}

static inline float ease_out_cubic(float t) {
    return 1.0f - powf(1.0f - t, 3.0f);
}

static inline float ease_out_cubic_lerp(float a, float b, float t) {
    return lerp(a, b, ease_out_cubic(t));
}

static inline float ease_in_out_quart(float t) {
    return t < 0.5f ? 8 * t * t * t * t : 1.0f - powf(-2.0f * t + 2, 4.0f) * 0.5f;
}

static inline float ease_in_out_quart_lerp(float a, float b, float t) {
    return lerp(a, b, ease_in_out_quart(t));
}

#endif // COMMON_H
