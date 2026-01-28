
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

void draw_ui(enum uiMode* mode) {
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
            uiButton(NULL, "New File");
            uiButton(NULL, "Open File");
            break;
    };

    uiEnd(&ui_info);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Draw to javascirpt");

    enum uiMode mode = UI_MODE_FILE_SELECTION;

    while (!WindowShouldClose()) {
        window_width = GetScreenWidth();
        window_height = GetScreenHeight();

        BeginDrawing();
        ClearBackground(BLACK);

        draw_ui(&mode);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

