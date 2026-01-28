
#include <stdio.h>

#include <raylib.h>

int main() {

    InitWindow(1200, 800, "Draw to javascirpt");
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
