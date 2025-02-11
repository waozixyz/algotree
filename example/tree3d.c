#include "raylib.h"
#include "raymath.h"
#define TREE3D_IMPLEMENTATION
#include "tree3d.h" 

int main(void) {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "3D Tree Example");

    // Define the camera to look into our 3D world
    Camera3D camera = {0};
    camera.position = (Vector3){15.0f, 15.0f, 15.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};          // Camera up vector
    camera.fovy = 45.0f;                              // Field of view (vertical)
    camera.projection = CAMERA_PERSPECTIVE;            // Perspective projection

    // Initialize the 3D tree
    Tree3D tree = Tree3DNewTree();
    
    // Base settings
    tree.X = 0.0f;
    tree.Y = 0.0f;
    tree.Z = 0.0f;
    tree.Width = 0.5f;
    tree.Height = 2.0f;
    tree.Scale = 1.0f;
    
    // Growth settings
    tree.MaxRow = 12;
    tree.LeafChance = 0.5f;
    tree.SplitChance = 50;
    tree.SplitAngle[0] = 20;
    tree.SplitAngle[1] = 30;
    tree.GrowTime = 10;
    
    // LOD settings
    tree.lodDistances[0] = 10.0f;
    tree.lodDistances[1] = 20.0f;
    tree.lodDistances[2] = 30.0f;
    tree.lodLevels[0] = 8;  // High detail
    tree.lodLevels[1] = 6;  // Medium detail
    tree.lodLevels[2] = 4;  // Low detail
    
    // Color settings
    tree.CsBranch[0] = 125; tree.CsBranch[1] = 178;  // Red range
    tree.CsBranch[2] = 122; tree.CsBranch[3] = 160;  // Green range
    tree.CsBranch[4] = 76;  tree.CsBranch[5] = 90;   // Blue range
    
    tree.CsLeaf[0] = 150; tree.CsLeaf[1] = 204;      // Red range
    tree.CsLeaf[2] = 190; tree.CsLeaf[3] = 230;      // Green range
    tree.CsLeaf[4] = 159; tree.CsLeaf[5] = 178;      // Blue range
    
    // Initialize memory and data structures
    Tree3DLoad(&tree);

    SetTargetFPS(60);

    // Camera control variables
    Vector2 previousMousePosition = {0.0f, 0.0f};
    float cameraAngle = 0.0f;
    float cameraDistance = 15.0f;

    // Main game loop
    while (!WindowShouldClose()) {
        // Update the tree
        Tree3DUpdate(&tree);

        // Camera rotation control
        Vector2 mousePosition = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float deltaX = mousePosition.x - previousMousePosition.x;
            cameraAngle += deltaX * 0.01f;
            
            camera.position.x = sinf(cameraAngle) * cameraDistance;
            camera.position.z = cosf(cameraAngle) * cameraDistance;
        }
        previousMousePosition = mousePosition;

        // Camera zoom control
        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0) {
            cameraDistance -= mouseWheel * 2.0f;
            if (cameraDistance < 5.0f) cameraDistance = 5.0f;
            if (cameraDistance > 30.0f) cameraDistance = 30.0f;
            
            camera.position.x = sinf(cameraAngle) * cameraDistance;
            camera.position.z = cosf(cameraAngle) * cameraDistance;
            camera.position.y = cameraDistance * 0.7f;
        }

        // Draw
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);

        // Draw ground grid
        DrawGrid(20, 1.0f);

        // Draw the 3D tree
        if (tree.needsBoundsUpdate) {
            Tree3DUpdateBounds(&tree);
        }
        Tree3DDraw(&tree, camera);  // Pass the camera to Tree3DDraw

        EndMode3D();

        // Draw UI
        DrawFPS(10, 10);
        DrawText("Left click and drag to rotate camera", 10, 30, 20, DARKGRAY);
        DrawText("Mouse wheel to zoom in/out", 10, 50, 20, DARKGRAY);
        DrawText("Press ESC to exit", 10, 70, 20, DARKGRAY);
        
        char debugInfo[100];
        sprintf(debugInfo, "Current Row: %d/%d", tree.CurrentRow, tree.MaxRow);
        DrawText(debugInfo, 10, 90, 20, DARKGRAY);

        EndDrawing();
    }
    
    // Cleanup
    Tree3DFree(&tree);
    CloseWindow();
    return 0;
}