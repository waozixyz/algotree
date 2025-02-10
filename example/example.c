#define TREE_IMPLEMENTATION
#include "tree.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Tree Example");
    SetTargetFPS(60);

    Tree sakura = new_tree();
    load(&sakura);

    while (!WindowShouldClose()) {
        update(&sakura);

        BeginDrawing();
        ClearBackground((Color){40, 7, 40, 255});
        draw(&sakura);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}