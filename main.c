
#include <stdio.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "libtinyfiledialogs/tinyfiledialogs.h"
#include "ui.h"

int32_t window_width = 1200;
int32_t window_height = 800;

enum uiMode {
    UI_MODE_FILE_SELECTION,
};

typedef struct Context {
    int32_t new_image_width;
    int32_t new_image_height;
} Context;

void new_image_menu(Context* ctx) {
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

    }

    bounds.x += menu_width * 0.33f;

    if (GuiButton(bounds, "Create")) {
        
    }
}

void draw_ui(enum uiMode* mode, Context* ctx) {
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

    switch (*mode) {
        case UI_MODE_FILE_SELECTION:
            static bool image_menu = false;
            if (uiButton(NULL, "New Image")) {
                image_menu = true;
            }
            uiButton(NULL, "Open Image");
            uiButton(NULL, "Open Javascript");
            if (image_menu) new_image_menu(ctx);
            break;
    };

    uiEnd(&ui_info);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT);
    InitWindow(window_width, window_height, "Draw to javascirpt");

    Context ctx = {0};

    enum uiMode mode = UI_MODE_FILE_SELECTION;
    Color clear_color = BLACK;

    while (!WindowShouldClose()) {
        window_width = GetScreenWidth();
        window_height = GetScreenHeight();

        BeginDrawing();
        ClearBackground(clear_color);

        draw_ui(&mode, &ctx);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

