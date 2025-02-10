#define TREE_IMPLEMENTATION
#include "tree2d.h"

int main() {
    // Initialize window dimensions
    const int screenWidth = 800;
    const int screenHeight = 600;

    // Initialize Raylib window
    InitWindow(screenWidth, screenHeight, "Tree Example");
    SetTargetFPS(60);

    // Create a new tree (e.g., a Sakura tree)
    Tree sakura = TreeNewTree();

    // Load the tree with default parameters
    TreeLoad(&sakura);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update the tree (grow branches and leaves)
        TreeUpdate(&sakura);

        // Begin drawing
        BeginDrawing();
        ClearBackground((Color){40, 7, 40, 255}); // Dark background color

        // Draw the tree
        TreeDraw(&sakura);

        EndDrawing();
    }

    // Close the Raylib window
    CloseWindow();

    return 0;
}