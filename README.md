# Algotree

Algotree is a lightweight single-header C library designed to quickly create procedurally generated trees in Raylib. It provides an easy-to-use API for generating and rendering tree structures with customizable parameters such as branch splits, leaf generation, and growth animations.

## Features
- **Single Header File**: The entire library is encapsulated in a single header file (`algotree.h`), making it easy to integrate into your projects.
- **Procedural Tree Generation**: Generate trees with customizable branching logic, split chances, and angles.
- **Dynamic Growth Animation**: Trees grow dynamically over time with smooth animations.
- **Customizable Colors**: Use color ranges to generate branches and leaves with randomized colors.
- **Cross-Platform**: Works on Linux, macOS, and Windows with Raylib.

## Usage
To use **Algotree**, simply include the `algotree.h` header file in your project and define the `ALGOTREE_IMPLEMENTATION` macro in one source file to include the implementation.

### Example Usage
Here’s how you can use **Algotree** to create and render a tree:

```c
#define ALGOTREE_IMPLEMENTATION
#include "algotree.h"

int main() {
    // Initialize Raylib window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Algotree Example");
    SetTargetFPS(60);

    // Create and load a tree
    Tree sakura = new_tree();
    load(&sakura);

    // Main game loop
    while (!WindowShouldClose()) {
        update(&sakura); // Update tree growth

        BeginDrawing();
        ClearBackground((Color){40, 7, 40, 255});
        draw(&sakura); // Draw the tree
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

### Compilation
Compile your program with Raylib linked. For example:
```bash
gcc example.c -o example -lraylib -lm
``

### Customization
You can customize the tree's appearance and behavior by modifying the `Tree` structure fields:
- `leaf_chance`: Probability of generating leaves.
- `max_row`: Maximum number of rows (levels) in the tree.
- `split_chance`: Chance of a branch splitting into two.
- `split_angle`: Range of angles for branch splits.
- `cs_branch` and `cs_leaf`: Color ranges for branches and leaves.

## Integration
1. Copy the `algotree.h` file into your project directory.
2. Include it in your source file(s):
   ```c
   #define ALGOTREE_IMPLEMENTATION
   #include "algotree.h"
   ```
3. Link against Raylib when compiling.

## License
**Algotree** is released under the BSD 3-Clause License. See the [LICENSE](LICENSE) file for details.

## Contributing
Contributions are welcome! If you find a bug or want to suggest an improvement, please open an issue or submit a pull request.

## Credits
- **Author**: WaoziXyz
- **Year**: 2025
- **Dependencies**: [Raylib](https://www.raylib.com/)
