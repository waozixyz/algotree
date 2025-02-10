#include "raylib.h"
#include "raymath.h"
#define TREE_IMPLEMENTATION
#include "tree3d.h" 

int main(void) {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "3D Tree Example");

    // Define the camera to look into our 3D world
    Camera3D camera = {0};
    camera.position = (Vector3){10.0f, 10.0f, 10.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                              // Field of view (vertical)
    camera.projection = CAMERA_PERSPECTIVE;          // Perspective projection

    // Initialize the 3D tree
    #define TREE_IMPLEMENTATION
    Tree3D tree = TreeNewTree3D();
    tree.X = 0.0f;
    tree.Y = 0.0f;
    tree.Z = 0.0f;
    tree.Width = 0.5f;
    tree.Height = 2.0f;
    tree.MaxRow = 12; // Maximum rows for the tree
    tree.LeafChance = 0.5f; // Chance for leaves to grow
    tree.SplitChance = 50; // Percentage chance for branches to split
    tree.SplitAngle[0] = 20; // Minimum split angle
    tree.SplitAngle[1] = 30; // Maximum split angle
    tree.GrowTime = 10; // Growth timer interval
    TreeLoad3D(&tree);

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    // Main game loop
    while (!WindowShouldClose()) {
        // Update the tree
        TreeUpdate3D(&tree);

        // Handle camera movement (WASD + mouse)
        if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(camera.target, 0.1f));
        if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(camera.target, 0.1f));
        if (IsKeyDown(KEY_A)) camera.position.x -= 0.1f;
        if (IsKeyDown(KEY_D)) camera.position.x += 0.1f;

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw the grid (ground)
        DrawGrid(10, 1.0f);

        // Draw the plane (ground)
        DrawPlane((Vector3){0.0f, -1.0f, 0.0f}, (Vector2){20.0f, 20.0f}, LIGHTGRAY);

        // Draw the 3D tree
        TreeDraw3D(&tree);

        EndMode3D();

        // Draw some debug text
        DrawFPS(10, 10);
        DrawText("Use WASD keys and mouse to move the camera!", 10, 30, 20, DARKGRAY);

        EndDrawing();
    }

    // Cleanup
    CloseWindow();
    return 0;
}