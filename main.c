
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "libtinyfiledialogs/tinyfiledialogs.h"
#include "ui.h"

#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof((arr)[0]))

int32_t window_width = 1200;
int32_t window_height = 800;

enum uiMode {
    UI_MODE_FILE_SELECTION,
    UI_MODE_IMAGE_EDITING,
    UI_MODE_EXPORT,
};

typedef struct Vector2I {
    int32_t x;
    int32_t y;
} Vector2I;

typedef struct Context {
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
    bool pick_color_draw;
    bool pick_color_ignore;
    Camera2D camera;
} Context;

void load_from_javascript(Context* ctx) {
    const char* filters[] = { "*.txt" };
    const char* path = tinyfd_openFileDialog(
            "Open Image",
            "",
            ARRAY_LEN(filters),
            filters,
            "Text Files",
            0);
    if (!path) {
        fprintf(stderr, "Failed to get path!\n");
        return;
    }

    FILE* file = fopen(path, "rb");
}

void new_image_menu(bool* stay_open, Context* ctx) {
    int32_t menu_width = window_width * 0.5f;
    int32_t menu_height = window_height * 0.5f;
    int32_t menu_start_x = window_width * 0.5f - menu_width * 0.5f;
    int32_t menu_start_y = window_height * 0.5f - menu_height * 0.5f;;
    Color color = DARKGRAY;
    color.r -= 20;
    color.g -= 20;
    color.b -= 20;
    DrawRectangle(menu_start_x, menu_start_y, menu_width, menu_height, color);
    
    const char* text = "New Image Menu";
    int32_t text_width = MeasureText(text, 35);
    DrawText(text, menu_start_x + menu_width * 0.5f - text_width * 0.5f, menu_start_y + menu_height * 0.1f, 35, RAYWHITE);

    Rectangle bounds = {
        .x = menu_start_x + menu_width * 0.33f,
        .y = menu_start_y + menu_height * 0.4f,
        .width = menu_width * 0.1f,
        .height = bounds.width,
    };

    bounds.x -= bounds.width * 0.5f;

    static bool new_image_width_edit_mode = false;
    if (GuiValueBox(bounds, "Width", &ctx->new_image_width, 1, 4000, new_image_width_edit_mode)) {
        new_image_width_edit_mode = !new_image_width_edit_mode;
    }

    bounds.x += menu_width * 0.33f;

    static bool new_image_height_edit_mode = false;
    if (GuiValueBox(bounds, "Height", &ctx->new_image_height, 1, 4000, new_image_height_edit_mode)) {
        new_image_height_edit_mode = !new_image_height_edit_mode;
    }

    bounds.width = menu_width * 0.25f;
    bounds.x = menu_start_x + menu_width * 0.33f - bounds.width * 0.5f;
    bounds.y += menu_height * 0.2f;
    if (GuiButton(bounds, "Close")) {
        *stay_open = false;
    }

    bounds.x += menu_width * 0.33f;

    if (GuiButton(bounds, "Create")) {
        ctx->image_data = malloc(ctx->new_image_width * ctx->new_image_height * 4);
        memset(ctx->image_data, 0, ctx->new_image_width * ctx->new_image_height * 4);
        if (!ctx->image_data) {
            fprintf(stderr, "Failed to create new Image\n");
            exit(1);
        };
        ctx->mode = UI_MODE_IMAGE_EDITING;
        *stay_open = false;

        Image img = GenImageColor(ctx->new_image_width, ctx->new_image_height, BLACK);

        ctx->loaded_tex = LoadTextureFromImage(img);
        UnloadImage(img);
        UpdateTexture(ctx->loaded_tex, ctx->image_data);
        ctx->loaded_ratio = (float)ctx->loaded_tex.width / (float)ctx->loaded_tex.height;
    }
}

static void image_to_javascript(Context* ctx, FILE* fd, char* name_x, char* name_y) {
    for (int32_t y = 0; y < ctx->new_image_height; y++) {
        for (int32_t x = 0; x < ctx->new_image_width; x++) {
            int32_t index = (y * ctx->new_image_width + x) * 4;
            
            uint8_t r = ctx->image_data[index];
            uint8_t g = ctx->image_data[index + 1];
            uint8_t b = ctx->image_data[index + 2];
            uint8_t a = ctx->image_data[index + 3];
            Color cmp_color = (Color){
                .r = r,
                .b = b,
                .g = g,
                .a = a,
            };
            uint32_t color = 0;
            color |= b;
            color |= g << 8;
            color |= r << 16;

            if (cmp_color.r == ctx->ignore_color.r &&
                    cmp_color.g == ctx->ignore_color.g &&
                    cmp_color.b == ctx->ignore_color.b &&
                    cmp_color.a == ctx->ignore_color.a) continue;

            int32_t pos_x = x - ctx->new_image_width / 2;
            if (ctx->export_x_mirrored) pos_x *= -1;
            fprintf(fd, "Canvas.rect(%s%+d, %s%+d, 1.5, 1.5, {fill:\"#%X\"}),\n", name_x, pos_x, name_y,
                    -(y - ctx->new_image_height / 2), color);
        }
    }
}

void draw_ui(Context* ctx) {
    uiInfo ui_info = {
        .start_x = window_width * 0.8f,
        .start_y = 0,
        .width = window_width * 0.2f,
        .height = window_height,
        .padding_x = ui_info.width * 0.1f,
        .padding_y = window_height * 0.05f,
        .element_height = window_height * 0.1f,
    };
    DrawRectangle(ui_info.start_x, 0, ui_info.width, window_height, DARKGRAY);
    
    uiBegin(&ui_info);

    switch (ctx->mode) {
        case UI_MODE_FILE_SELECTION:
            static bool image_menu = false;
            if (uiButton(NULL, "New Image")) {;
                image_menu = true;
            }
            if (uiButton(NULL, "Open Image")) {
                const char* filters[] = { "*.png", "*.jpg", "*.jpeg" };
                const char* path = tinyfd_openFileDialog(
                        "Open Image",
                        "",
                        ARRAY_LEN(filters),
                        filters,
                        "Images",
                        0);
                if (path) {
                    int32_t channels = 0;
                    ctx->image_data = stbi_load(path, &ctx->new_image_width, &ctx->new_image_height, &channels, 4);
                    if (!ctx->image_data) {
                        fprintf(stderr, "Failed to load image: %s\n", path);
                        exit(1);
                    }
                    ctx->mode = UI_MODE_IMAGE_EDITING;
                    Image img = GenImageColor(ctx->new_image_width, ctx->new_image_height, BLACK);

                    ctx->loaded_tex = LoadTextureFromImage(img);
                    UnloadImage(img);
                    UpdateTexture(ctx->loaded_tex, ctx->image_data);
                    ctx->loaded_ratio = (float)ctx->loaded_tex.width / (float)ctx->loaded_tex.height;
                }
                else {
                    fprintf(stderr, "Failed selecting a file!\n");
                }
            }
            uiButton(NULL, "Open Javascript");
            if (image_menu) new_image_menu(&image_menu, ctx);
            break;
        case UI_MODE_IMAGE_EDITING:
            uiSlider(NULL, "Brush Size", NULL, &ctx->brush_size, 0.5f, 50.0f);

            uiTextEx(NULL, "Brush Color", RAYWHITE, false);
            uiColorPicker(NULL, "Color Picker", &ctx->draw_color);
            if (uiButton(NULL, "Pick Color")) {
                ctx->pick_color_draw = true;
            }

            uiTextEx(NULL, "Ignore Color", RAYWHITE, false);
            uiColorPicker(NULL, "Color Picker", &ctx->ignore_color);
            if (uiButton(NULL, "Pick Color")) {
                ctx->pick_color_ignore = true;
            }

            if (uiButton(NULL, "Export Tab")) {
                ctx->mode = UI_MODE_EXPORT;
            }
            break;
        case UI_MODE_EXPORT:
            if (uiButton(NULL, "Image Editing")) {
                ctx->mode = UI_MODE_IMAGE_EDITING;
            }

            static bool text_box_pos_x_edit_mode = false;
            size_t text_box_pos_x_size = 256;
            static char text_box_pos_x_input[256];
            if (uiTextBox(NULL, text_box_pos_x_input, text_box_pos_x_size, text_box_pos_x_edit_mode)) {
                text_box_pos_x_edit_mode = !text_box_pos_x_edit_mode;
            }

            static bool text_box_pos_y_edit_mode = false;
            size_t text_box_pos_y_size = 256;
            static char text_box_pos_y_input[256];
            if (uiTextBox(NULL, text_box_pos_y_input, text_box_pos_y_size, text_box_pos_y_edit_mode)) {
                text_box_pos_y_edit_mode = !text_box_pos_y_edit_mode;
            }

            uiCheckBox(NULL, "Gespiegelt X", &ctx->export_x_mirrored);


            if (uiButton(NULL, "Export to Javascript")) {
                const char* filters[] = { "*.txt", "*.js" };
                const char* path = tinyfd_saveFileDialog(
                    "",
                    "",
                    ARRAY_LEN(filters),
                    filters,
                    "Text or Js files");
                if (!path) {
                    fprintf(stderr, "Failed saving file!\n");
                    uiEnd(&ui_info);
                    return;
                }
                FILE* file = fopen(path, "wb");
                if (!file) {
                    fprintf(stderr, "Failed to open file: %s\n", path);
                    uiEnd(&ui_info);
                }
                image_to_javascript(ctx, file, text_box_pos_x_input, text_box_pos_y_input);
                fclose(file);
            }
            if (uiButton(NULL, "Export to Image")) {
                const char* filters[] = { "*.txt", "*.js" };
                const char* path = tinyfd_saveFileDialog(
                        "",
                        "",
                        ARRAY_LEN(filters),
                        filters,
                        "Image Files");
                if (!path) {
                    fprintf(stderr, "Failed saving file!\n");
                    uiEnd(&ui_info);
                    return;
                }
                stbi_write_png(path, ctx->new_image_width, ctx->new_image_height, 4, ctx->image_data, ctx->new_image_width * 4);
            }
            break;
    };

    uiEnd(&ui_info);
}

static Rectangle get_image_dst(Context* ctx)
{
    Rectangle dst = {
        .x = 0,
        .y = 0,
        .width = window_width * 0.8f,
    };

    dst.height = dst.width / ctx->loaded_ratio;

    if (dst.height > window_height) {
        dst.height = window_height;
        dst.width = dst.height * ctx->loaded_ratio;
    }

    return dst;
}

void draw_image(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    Rectangle src = {
        .width = ctx->loaded_tex.width,
        .height = ctx->loaded_tex.height,
    };

    Rectangle dst = get_image_dst(ctx);

    DrawTexturePro(ctx->loaded_tex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLines(0, 0, dst.width, dst.height, RAYWHITE);
}

static inline Vector2I screen_to_image_space(Context* ctx, Vector2 vec, Rectangle dst) {
    float u = (vec.x - dst.x) / dst.width;
    float v = (vec.y - dst.y) / dst.height;

    u = fminf(fmaxf(u, 0.0f), 1.0f);
    v = fminf(fmaxf(v, 0.0f), 1.0f);

    int cx = (int32_t)floorf(u * ctx->new_image_width);
    int cy = (int32_t)floorf(v * ctx->new_image_height);
    return (Vector2I){cx, cy};
}

static inline int32_t vec_to_img(Context* ctx, Vector2I vec) {
    if (vec.x < 0 || vec.y < 0 ||
        vec.x >= ctx->new_image_width ||
        vec.y >= ctx->new_image_height)
        return -1;

    return (vec.y * ctx->new_image_width + vec.x) * 4;
}

void update_image_data(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Rectangle dst = get_image_dst(ctx);
        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), ctx->camera);
    
        if (!CheckCollisionPointRec(mouse, dst)) return;

        if (ctx->pick_color_draw) {
            Vector2I pos = screen_to_image_space(ctx, mouse, dst);
            int32_t index = vec_to_img(ctx, pos);
            ctx->draw_color.r = ctx->image_data[index];
            ctx->draw_color.g = ctx->image_data[index + 1];
            ctx->draw_color.b = ctx->image_data[index + 2];
            return;
        }

        if (ctx->pick_color_ignore) {
            Vector2I pos = screen_to_image_space(ctx, mouse, dst);
            int32_t index = vec_to_img(ctx, pos);
            ctx->ignore_color.r = ctx->image_data[index];
            ctx->ignore_color.g = ctx->image_data[index + 1];
            ctx->ignore_color.b = ctx->image_data[index + 2];
            return;
        }

        float radius = ctx->brush_size;
        float radius_squared = radius * radius;

        for (int32_t i = -radius; i <= radius; i++) {
            for (int32_t j = -radius; j <= radius; j++) {
                Vector2I pos = screen_to_image_space(ctx, mouse, dst);
                pos.x += i;
                pos.y += j;
                if (i * i + j * j <= radius_squared) {
                    int32_t index = vec_to_img(ctx, pos);
                    if (index == -1) continue;
                    ctx->image_data[index] = ctx->draw_color.r;
                    ctx->image_data[index + 1] = ctx->draw_color.g;
                    ctx->image_data[index + 2] = ctx->draw_color.b;
                    ctx->image_data[index + 3] = 255;
                }
            }
        }

        UpdateTexture(ctx->loaded_tex, ctx->image_data);
    }
}

static void handle_input(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    
    // Panning
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / ctx->camera.zoom);
        ctx->camera.target = Vector2Add(ctx->camera.target, delta);
    }

    // Zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouse_world_pos = GetScreenToWorld2D(GetMousePosition(), ctx->camera);
        ctx->camera.offset = GetMousePosition();
        ctx->camera.target = mouse_world_pos;

        float scale = wheel * 0.2f;
        ctx->camera.zoom = Clamp(expf(logf(ctx->camera.zoom) + scale), 0.125f, 64.0f);
    }
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT);
    InitWindow(window_width, window_height, "Draw to javascirpt");

    Context ctx = {0};
    ctx.mode = UI_MODE_FILE_SELECTION;
    ctx.clear_color = BLACK;
    ctx.draw_color = RED;
    ctx.brush_size = 2.0f;
    ctx.camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        window_width = GetScreenWidth();
        window_height = GetScreenHeight();
        float fps = 1.0f / GetFrameTime();

        handle_input(&ctx);
        update_image_data(&ctx);

        BeginDrawing();
        ClearBackground(ctx.clear_color);
        BeginMode2D(ctx.camera);

        draw_image(&ctx);
        DrawText(TextFormat("FPS: %.2f", fps), 10, 10, 20, RAYWHITE);

        EndMode2D();
        draw_ui(&ctx);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

