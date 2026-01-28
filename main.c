
#include <stdio.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "libtinyfiledialogs/tinyfiledialogs.h"

int main() {

    InitWindow(1200, 800, "Draw to javascirpt");
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle bounds = {
            .x = 20,
            .y = 20,
            .width = 100,
            .height = 20,
        };
        GuiButton(bounds, "Button");
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

