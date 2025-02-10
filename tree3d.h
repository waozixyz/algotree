#ifndef TREE3D_H
#define TREE3D_H

#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// Configuration Macros
#ifndef MAX_ROWS
#define MAX_ROWS 100
#endif

#ifndef MAX_BRANCHES_PER_ROW
#define MAX_BRANCHES_PER_ROW 1000
#endif

#ifndef MAX_LEAVES
#define MAX_LEAVES 10000
#endif

// Define this macro in ONE source file to include the implementation
#ifdef TREE3D_IMPLEMENTATION
#define TREE3D_IMPL
#endif

// Struct Definitions
typedef struct {
    int DegX;
    int DegZ;
    Vector3 V1;
    Vector3 V2;
    float Width;
    float Height;
    Color Color;
} Tree3DBranch;

typedef struct {
    size_t Row;
    Vector3 V1;
    Vector3 V2;
    float Radius;
    Color Color;
} Tree3DLeaf;

typedef struct {
    Tree3DBranch **Branches;  // Dynamic 2D array
    int *BranchCount;         // Dynamic array
    Tree3DLeaf *Leaves;       // Dynamic array
    int LeafCount;
    float LeafChance;
    int MaxRow;
    float X;
    float Y;
    float Z;
    int CurrentRow;
    bool RandomRow;
    int SplitChance;
    int SplitAngle[2];
    unsigned char CsBranch[6];
    unsigned char CsLeaf[6];
    float MinX;
    float MaxX;
    float MinZ;
    float MaxZ;
    int GrowTimer;
    int GrowTime;
    float Width;
    float Height;
    // Add allocation tracking
    int AllocatedRows;
    int AllocatedBranchesPerRow;
    int AllocatedLeaves;
} Tree3D;

// Function Declarations
Tree3D Tree3DNewTree();
void Tree3DLoad(Tree3D *tree);
void Tree3DUpdate(Tree3D *tree);
void Tree3DDraw(Tree3D *tree);

#ifdef TREE3D_IMPL

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG_TO_RAD (M_PI / 180.0)

// Helper Functions
Color Tree3DGetColor(unsigned char cs[6]) {
    unsigned char r = rand() % (cs[1] - cs[0] + 1) + cs[0];
    unsigned char g = rand() % (cs[3] - cs[2] + 1) + cs[2];
    unsigned char b = rand() % (cs[5] - cs[4] + 1) + cs[4];
    return (Color){r, g, b, 255};
}

void Tree3DAppendRow(Tree3D *tree) {
    tree->BranchCount[tree->CurrentRow + 1] = 0;
}

void Tree3DAppendBranch(Tree3D *tree, int row, Tree3DBranch branch) {
    if (tree->BranchCount[row] < MAX_BRANCHES_PER_ROW) {
        tree->Branches[row][tree->BranchCount[row]++] = branch;
    } else {
        fprintf(stderr, "Error: Maximum branches per row exceeded.\n");
    }
}

void Tree3DAppendLeaf(Tree3D *tree, Tree3DLeaf leaf) {
    if (tree->LeafCount < MAX_LEAVES) {
        tree->Leaves[tree->LeafCount++] = leaf;
    } else {
        fprintf(stderr, "Error: Maximum leaves exceeded.\n");
    }
}

int Tree3DGetAngle(Tree3D *tree) {
    return rand() % (tree->SplitAngle[1] - tree->SplitAngle[0] + 1) + tree->SplitAngle[0];
}
Vector3 Tree3DGetRotation(int degX, int degZ) {
    // Convert angles to radians
    float radX = degX * DEG_TO_RAD;
    float radZ = degZ * DEG_TO_RAD;

    // Calculate direction vector
    // Start with a vector pointing up (0,1,0)
    // Then rotate around X and Z axes
    Vector3 direction = {
        sinf(radZ),                          // X component
        cosf(radX) * cosf(radZ),            // Y component (upward growth)
        sinf(radX)                          // Z component
    };

    return direction;
}
void Tree3DAddBranch(Tree3D *tree, int degX, int degZ, Tree3DBranch *branch) {
    float w = branch->Width * 0.9;
    float h = branch->Height * 0.95;
    Vector3 pos = branch->V2;
    Vector3 rot = Tree3DGetRotation(degX, degZ);
    Vector3 newPos = {
        pos.x + rot.x * h,
        pos.y + rot.y * h,
        pos.z + rot.z * h
    };
    Color c = Tree3DGetColor(tree->CsBranch);
    
    Tree3DBranch newBranch = {
        .DegX = degX,
        .DegZ = degZ,
        .V1 = pos,
        .V2 = newPos,
        .Width = w,
        .Height = h,
        .Color = c
    };
    Tree3DAppendBranch(tree, tree->CurrentRow + 1, newBranch);

    float leafChance = ((float)rand() / RAND_MAX) * tree->CurrentRow / tree->MaxRow;
    if (leafChance > tree->LeafChance) {
        Vector3 rotLeaf = Tree3DGetRotation(degX * 2, degZ * 2);
        Vector3 leafOffset = {rotLeaf.x * w, rotLeaf.y * w, rotLeaf.z * w};
        Tree3DLeaf newLeaf = {
            .Row = tree->CurrentRow,
            .Radius = w,
            .V1 = (Vector3){newPos.x + leafOffset.x, newPos.y + leafOffset.y, newPos.z + leafOffset.z},
            .V2 = (Vector3){newPos.x - leafOffset.x, newPos.y - leafOffset.y, newPos.z - leafOffset.z},
            .Color = Tree3DGetColor(tree->CsLeaf)
        };
        Tree3DAppendLeaf(tree, newLeaf);
    }

    if (newPos.x < tree->MinX) tree->MinX = newPos.x;
    if (newPos.x > tree->MaxX) tree->MaxX = newPos.x + w;
    if (newPos.z < tree->MinZ) tree->MinZ = newPos.z;
    if (newPos.z > tree->MaxZ) tree->MaxZ = newPos.z + w;
}

float Tree3DGetNextPos(Tree3D *tree, float a, float b) {
    return b + (a - b) * tree->GrowTimer / (float)tree->GrowTime;
}

void Tree3DGrow(Tree3D *tree) {
    Tree3DAppendRow(tree);
    int prevRow = tree->CurrentRow;
    for (int i = 0; i < tree->BranchCount[prevRow]; i++) {
        Tree3DBranch *b = &tree->Branches[prevRow][i];
        int split = rand() % 100;
        if (tree->SplitChance > split) {
            int angleX = Tree3DGetAngle(tree);
            int angleZ = Tree3DGetAngle(tree);
            Tree3DAddBranch(tree, b->DegX - angleX, b->DegZ - angleZ, b);
            Tree3DAddBranch(tree, b->DegX + angleX, b->DegZ + angleZ, b);
        } else {
            Tree3DAddBranch(tree, b->DegX, b->DegZ, b);
        }
    }
    tree->CurrentRow++;
}
void Tree3DLoad(Tree3D *tree) {
    Tree3DAppendRow(tree);
    Tree3DBranch initialBranch = {
        .DegX = 0,
        .DegZ = 0,
        .V1 = (Vector3){tree->X, tree->Y, tree->Z},
        .V2 = (Vector3){tree->X, tree->Y + tree->Height, tree->Z},  // Start growing upward
        .Width = tree->Width,
        .Height = tree->Height,
        .Color = Tree3DGetColor(tree->CsBranch)  // Use branch color system instead of WHITE
    };
    Tree3DAppendBranch(tree, 0, initialBranch);
    tree->GrowTimer = rand() % tree->GrowTime;
    if (tree->RandomRow) {
        int growToRow = rand() % tree->MaxRow;
        while (tree->CurrentRow < growToRow) {
            Tree3DGrow(tree);
        }
    }
}

void Tree3DUpdate(Tree3D *tree) {
    if (tree->GrowTimer > 0) tree->GrowTimer--;
    if (tree->GrowTimer == 0 && tree->CurrentRow < tree->MaxRow) {
        Tree3DGrow(tree);
        tree->GrowTimer = tree->GrowTime;
    }
}

void Tree3DDraw(Tree3D *tree) {
    for (int i = 0; i <= tree->CurrentRow; i++) {
        for (int j = 0; j < tree->BranchCount[i]; j++) {
            Tree3DBranch *b = &tree->Branches[i][j];
            Vector3 v2 = b->V2;
            if (i == tree->CurrentRow && tree->GrowTimer > 0) {
                v2 = (Vector3){
                    Tree3DGetNextPos(tree, b->V1.x, v2.x),
                    Tree3DGetNextPos(tree, b->V1.y, v2.y),
                    Tree3DGetNextPos(tree, b->V1.z, v2.z)
                };
            }
            DrawCylinderEx(b->V1, v2, b->Width, b->Width * 0.8f, 8, b->Color);
        }
        for (int j = 0; j < tree->LeafCount; j++) {
            Tree3DLeaf *l = &tree->Leaves[j];
            if ((int)l->Row < i && !(i == tree->CurrentRow && tree->GrowTimer > 0)) {
                DrawSphere(l->V1, l->Radius, l->Color);
                DrawSphere(l->V2, l->Radius, l->Color);
            }
        }
    }
}

// Modified initialization function
Tree3D Tree3DNewTree() {
    printf("Starting Tree3DNewTree\n");
    
    Tree3D tree = {0};  // Zero initialize basic members
    
    // Allocate memory for arrays
    tree.AllocatedRows = MAX_ROWS;
    tree.AllocatedBranchesPerRow = MAX_BRANCHES_PER_ROW;
    tree.AllocatedLeaves = MAX_LEAVES;
    
    // Allocate BranchCount array
    tree.BranchCount = (int*)calloc(tree.AllocatedRows, sizeof(int));
    if (!tree.BranchCount) {
        fprintf(stderr, "Failed to allocate BranchCount array\n");
        exit(1);
    }
    
    // Allocate Branches 2D array
    tree.Branches = (Tree3DBranch**)malloc(tree.AllocatedRows * sizeof(Tree3DBranch*));
    if (!tree.Branches) {
        fprintf(stderr, "Failed to allocate Branches array\n");
        free(tree.BranchCount);
        exit(1);
    }
    
    for (int i = 0; i < tree.AllocatedRows; i++) {
        tree.Branches[i] = (Tree3DBranch*)malloc(tree.AllocatedBranchesPerRow * sizeof(Tree3DBranch));
        if (!tree.Branches[i]) {
            fprintf(stderr, "Failed to allocate Branches row %d\n", i);
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++) {
                free(tree.Branches[j]);
            }
            free(tree.Branches);
            free(tree.BranchCount);
            exit(1);
        }
    }
    
    // Allocate Leaves array
    tree.Leaves = (Tree3DLeaf*)malloc(tree.AllocatedLeaves * sizeof(Tree3DLeaf));
    if (!tree.Leaves) {
        fprintf(stderr, "Failed to allocate Leaves array\n");
        for (int i = 0; i < tree.AllocatedRows; i++) {
            free(tree.Branches[i]);
        }
        free(tree.Branches);
        free(tree.BranchCount);
        exit(1);
    }
    
    // Initialize basic values
    tree.LeafCount = 0;
    tree.LeafChance = 0.5f;
    tree.MaxRow = 12;
    tree.CurrentRow = 0;
    tree.X = 0.0f;
    tree.Y = 0.0f;
    tree.Z = 0.0f;
    tree.Width = 1.0f;
    tree.Height = 4.0f;
    tree.RandomRow = false;
    tree.SplitChance = 50;
    tree.SplitAngle[0] = 20;
    tree.SplitAngle[1] = 30;
    
    // Color settings
    tree.CsBranch[0] = 125; tree.CsBranch[1] = 178;
    tree.CsBranch[2] = 122; tree.CsBranch[3] = 160;
    tree.CsBranch[4] = 76;  tree.CsBranch[5] = 90;
    
    tree.CsLeaf[0] = 150; tree.CsLeaf[1] = 204;
    tree.CsLeaf[2] = 190; tree.CsLeaf[3] = 230;
    tree.CsLeaf[4] = 159; tree.CsLeaf[5] = 178;
    
    // Bounds
    tree.MinX = 9999999.0f;
    tree.MaxX = -9999999.0f;
    tree.MinZ = 9999999.0f;
    tree.MaxZ = -9999999.0f;
    
    // Timers
    tree.GrowTimer = 0;
    tree.GrowTime = 20;
    
    return tree;
}

// Add cleanup function
void Tree3DFree(Tree3D *tree) {
    if (!tree) return;
    
    if (tree->Branches) {
        for (int i = 0; i < tree->AllocatedRows; i++) {
            if (tree->Branches[i]) {
                free(tree->Branches[i]);
            }
        }
        free(tree->Branches);
        tree->Branches = NULL;
    }
    
    if (tree->BranchCount) {
        free(tree->BranchCount);
        tree->BranchCount = NULL;
    }
    
    if (tree->Leaves) {
        free(tree->Leaves);
        tree->Leaves = NULL;
    }
}

#endif // TREE3D_IMPL
#endif // TREE3D_H