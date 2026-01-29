
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
};

typedef struct Context {
    int32_t new_image_width;
    int32_t new_image_height;
    uint8_t* image_data;
    Texture2D loaded_tex;
    float loaded_ratio;
    enum uiMode mode;
    Color clear_color;
} Context;

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

void draw_ui(Context* ctx) {
    uiInfo ui_info = {
        .start_x = window_width * 0.8f,
        .start_y = 0,
        .width = window_width * 0.2f,
        .height = window_height,
        .padding_x = ui_info.width * 0.1f,
        .padding_y = window_height * 0.1f,
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
            uiButton(NULL, "Image Editing");
            break;
    };

    uiEnd(&ui_info);
}

Rectangle get_image_dst(Context* ctx)
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
}

static inline int32_t vector_to_index(Context* ctx, Vector2 vec, Rectangle dst) {
    float u = (vec.x - dst.x) / dst.width;
    float v = (vec.y - dst.y) / dst.height;

    int32_t x = (int32_t)(u * ctx->new_image_width);
    int32_t y = (int32_t)(v * ctx->new_image_height);

    if (x < 0 || y < 0 ||
        x >= ctx->new_image_width ||
        y >= ctx->new_image_height) {
        fprintf(stderr, "Warning: writing out of image bounds\n");
        return 0;
    }

    int32_t index = (y * ctx->new_image_width + x) * 4;
    return index;
}

static float distance_to_circle(Vector2 center, Vector2 pos) {
    float radius = 25.0f;
    float dist = Vector2Distance(center, pos);
    return dist - radius;
}

void update_image_data(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Rectangle dst = get_image_dst(ctx);
        Vector2 mouse = GetMousePosition();
    
        if (!CheckCollisionPointRec(mouse, dst)) return;

        float radius = 25.0f;
        float radius_half = radius * 0.5f;

        for (int32_t i = -radius; i < radius; i++) {
            for (int32_t j = -radius; j < radius; j++) {
                Vector2 pos = mouse;
                pos.x += i;
                pos.y += j;
                float dist = distance_to_circle(mouse, pos);
                if (dist <= 0.0f) {
                    int32_t index = vector_to_index(ctx, pos, dst);
                    ctx->image_data[index] = 255;
                    ctx->image_data[index + 1] = 0;
                    ctx->image_data[index + 2] = 0;
                    ctx->image_data[index + 3] = 255;
                }
            }
        }

        /*
        for (int32_t i = 0; i < radius; i++) {
            for (float j = 0.0f; j < 2 * PI; j += 0.01f) {
                float x = cos(j) * i;
                float y = sin(j) * i;
                Vector2 pos = mouse;
                pos.x += x;
                pos.y += y;
                int32_t index = vector_to_index(ctx, pos, dst);
                ctx->image_data[index] = 255;
                ctx->image_data[index + 1] = 0;
                ctx->image_data[index + 2] = 0;
                ctx->image_data[index + 3] = 255;
            }
        }
        */

        //int32_t index = vector_to_index(ctx, mouse, dst);


        UpdateTexture(ctx->loaded_tex, ctx->image_data);
    }

}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT);
    InitWindow(window_width, window_height, "Draw to javascirpt");

    Context ctx = {0};
    ctx.mode = UI_MODE_FILE_SELECTION;
    ctx.clear_color = BLACK;

    while (!WindowShouldClose()) {
        window_width = GetScreenWidth();
        window_height = GetScreenHeight();

        update_image_data(&ctx);

        BeginDrawing();
        ClearBackground(ctx.clear_color);

        draw_image(&ctx);
        draw_ui(&ctx);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

