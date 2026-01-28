
#include <stdio.h>

#include <raylib.h>

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
    int32_t new_image_channels;
    uint8_t* image_data;
    bool loaded;
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
        .x = menu_start_x + menu_width * 0.25f,
        .y = menu_start_y + menu_height * 0.4f,
        .width = menu_width * 0.1f,
        .height = bounds.width,
    };

    bounds.x -= bounds.width * 0.5f;

    static bool new_image_width_edit_mode = false;
    if (GuiValueBox(bounds, "Width", &ctx->new_image_width, 1, 4000, new_image_width_edit_mode)) {
        new_image_width_edit_mode = !new_image_width_edit_mode;
    }

    bounds.x += menu_width * 0.25f;

    static bool new_image_height_edit_mode = false;
    if (GuiValueBox(bounds, "Height", &ctx->new_image_height, 1, 4000, new_image_height_edit_mode)) {
        new_image_height_edit_mode = !new_image_height_edit_mode;
    }
    
    bounds.x += menu_width * 0.25f;

    static bool new_image_channels_edit_mode = false;
    if (GuiValueBox(bounds, "Channels", &ctx->new_image_channels, 1, 4, new_image_channels_edit_mode)) {
        new_image_channels_edit_mode = !new_image_channels_edit_mode;
    }

    bounds.width = menu_width * 0.25f;
    bounds.x = menu_start_x + menu_width * 0.33f - bounds.width * 0.5f;
    bounds.y += menu_height * 0.2f;
    if (GuiButton(bounds, "Close")) {
        *stay_open = false;
    }

    bounds.x += menu_width * 0.33f;

    if (GuiButton(bounds, "Create")) {
        ctx->image_data = malloc(ctx->new_image_width * ctx->new_image_height * ctx->new_image_channels);
        if (!ctx->image_data) {
            fprintf(stderr, "Failed to create new Image\n");
            exit(1);
        };
        ctx->mode = UI_MODE_IMAGE_EDITING;
        *stay_open = false;
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
                    if (channels != 4) {
                        fprintf(stderr, "Error image channels != 4");
                        exit(1); // TMP
                     }
                    ctx->mode = UI_MODE_IMAGE_EDITING;
                    ctx->loaded = true;
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

void draw_image(Context* ctx) {
    if (ctx->loaded) {
        Rectangle src = {
            .width = ctx->loaded_tex.width,
            .height = ctx->loaded_tex.height,
        };

        Rectangle dst = {
            .width = window_width * 0.8f,
            .height = dst.width / ctx->loaded_ratio,
        };

        if (dst.height > window_height) {
            dst.height = window_height;
            dst.width = dst.height * ctx->loaded_ratio;
        }

        DrawTexturePro(ctx->loaded_tex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

static inline int32_t vector_to_index(Context* ctx, Vector2 vec) {
    int32_t index = ((int32_t)vec.y * ctx->new_image_width + (int32_t)vec.x) * ctx->new_image_channels;
    if (index < 0 || index < ctx->new_image_width * ctx->new_image_height * ctx->new_image_channels) {
        fprintf(stderr, "WARNING invalid index\n");
        return 0;
    }
    return index;
}

void update_image_data(Context* ctx) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int32_t index = vector_to_index(ctx, GetMousePosition());
        ctx->image_data[index] = 255;

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

