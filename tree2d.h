#ifndef TREE_H
#define TREE_H

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
#ifdef TREE_IMPLEMENTATION
#define TREE_IMPL
#endif

// Struct Definitions
typedef struct {
    int Deg;
    Vector2 V1;
    Vector2 V2;
    float Width;
    float Height;
    Color Color;
} TreeBranch;

typedef struct {
    size_t Row;
    Vector2 V1;
    Vector2 V2;
    float Radius;
    Color Color;
} TreeLeaf;

typedef struct {
    TreeBranch Branches[MAX_ROWS][MAX_BRANCHES_PER_ROW];
    int BranchCount[MAX_ROWS];
    TreeLeaf Leaves[MAX_LEAVES];
    int LeafCount;
    float LeafChance;
    int MaxRow;
    float X;
    float Y;
    int CurrentRow;
    bool RandomRow;
    int SplitChance;
    int SplitAngle[2];
    unsigned char CsBranch[6];
    unsigned char CsLeaf[6];
    float LeftX;
    float RightX;
    int GrowTimer;
    int GrowTime;
    float Width;
    float Height;
} Tree;

// Function Declarations
Tree TreeNewTree();
void TreeLoad(Tree *tree);
void TreeUpdate(Tree *tree);
void TreeDraw(Tree *tree);

#ifdef TREE_IMPL

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG_TO_RAD (M_PI / 180.0)

// Helper Functions
Color TreeGetColor(unsigned char cs[6]) {
    unsigned char r = rand() % (cs[1] - cs[0] + 1) + cs[0];
    unsigned char g = rand() % (cs[3] - cs[2] + 1) + cs[2];
    unsigned char b = rand() % (cs[5] - cs[4] + 1) + cs[4];
    return (Color){r, g, b, 255};
}

void TreeAppendRow(Tree *tree) {
    tree->BranchCount[tree->CurrentRow + 1] = 0;
}

void TreeAppendBranch(Tree *tree, int row, TreeBranch branch) {
    if (tree->BranchCount[row] < MAX_BRANCHES_PER_ROW) {
        tree->Branches[row][tree->BranchCount[row]++] = branch;
    } else {
        fprintf(stderr, "Error: Maximum branches per row exceeded.\n");
    }
}

void TreeAppendLeaf(Tree *tree, TreeLeaf leaf) {
    if (tree->LeafCount < MAX_LEAVES) {
        tree->Leaves[tree->LeafCount++] = leaf;
    } else {
        fprintf(stderr, "Error: Maximum leaves exceeded.\n");
    }
}

int TreeGetAngle(Tree *tree) {
    return rand() % (tree->SplitAngle[1] - tree->SplitAngle[0] + 1) + tree->SplitAngle[0];
}

float TreeGetRotX(int deg) {
    return cosf(deg * DEG_TO_RAD);
}

float TreeGetRotY(int deg) {
    return sinf(deg * DEG_TO_RAD);
}

void TreeAddBranch(Tree *tree, int deg, TreeBranch *branch) {
    float w = branch->Width * 0.9;
    float h = branch->Height * 0.95;
    float px = branch->V2.x;
    float py = branch->V2.y;
    float nx = px + TreeGetRotX(deg) * h;
    float ny = py + TreeGetRotY(deg) * h;
    Color c = TreeGetColor(tree->CsBranch);
    TreeBranch newBranch = {
        .Deg = deg,
        .V1 = (Vector2){px, py},
        .V2 = (Vector2){nx, ny},
        .Width = w,
        .Height = h,
        .Color = c
    };
    TreeAppendBranch(tree, tree->CurrentRow + 1, newBranch);

    float leafChance = ((float)rand() / RAND_MAX) * tree->CurrentRow / tree->MaxRow;
    if (leafChance > tree->LeafChance) {
        float divX = TreeGetRotX(deg * 2) * w;
        float divY = TreeGetRotY(deg * 2) * w;
        TreeLeaf newLeaf = {
            .Row = tree->CurrentRow,
            .Radius = w,
            .V1 = (Vector2){nx + divX, ny + divY},
            .V2 = (Vector2){nx - divX, ny - divY},
            .Color = TreeGetColor(tree->CsLeaf)
        };
        TreeAppendLeaf(tree, newLeaf);
    }

    if (nx < tree->LeftX) tree->LeftX = nx;
    if (nx > tree->RightX) tree->RightX = nx + w;
}

float TreeGetNextPos(Tree *tree, float a, float b) {
    return b + (a - b) * tree->GrowTimer / (float)tree->GrowTime;
}

void TreeGrow(Tree *tree) {
    TreeAppendRow(tree);
    int prevRow = tree->CurrentRow;
    for (int i = 0; i < tree->BranchCount[prevRow]; i++) {
        TreeBranch *b = &tree->Branches[prevRow][i];
        int split = rand() % 100;
        if (tree->SplitChance > split) {
            TreeAddBranch(tree, b->Deg - TreeGetAngle(tree), b);
            TreeAddBranch(tree, b->Deg + TreeGetAngle(tree), b);
        } else {
            TreeAddBranch(tree, b->Deg, b);
        }
    }
    tree->CurrentRow++;
}

void TreeLoad(Tree *tree) {
    int angle = -90;
    TreeAppendRow(tree);
    TreeBranch initialBranch = {
        .Deg = angle,
        .V1 = (Vector2){tree->X, tree->Y},
        .V2 = (Vector2){tree->X, tree->Y},
        .Width = tree->Width,
        .Height = tree->Height,
        .Color = WHITE
    };
    TreeAppendBranch(tree, 0, initialBranch);
    tree->GrowTimer = rand() % tree->GrowTime;
    if (tree->RandomRow) {
        int growToRow = rand() % tree->MaxRow;
        while (tree->CurrentRow < growToRow) {
            TreeGrow(tree);
        }
    }
}

void TreeUpdate(Tree *tree) {
    if (tree->GrowTimer > 0) tree->GrowTimer--;
    if (tree->GrowTimer == 0 && tree->CurrentRow < tree->MaxRow) {
        TreeGrow(tree);
        tree->GrowTimer = tree->GrowTime;
    }
}

void TreeDraw(Tree *tree) {
    for (int i = 0; i <= tree->CurrentRow; i++) {
        for (int j = 0; j < tree->BranchCount[i]; j++) {
            TreeBranch *b = &tree->Branches[i][j];
            Vector2 v2 = b->V2;
            if (i == tree->CurrentRow && tree->GrowTimer > 0) {
                v2 = (Vector2){
                    .x = TreeGetNextPos(tree, b->V1.x, v2.x),
                    .y = TreeGetNextPos(tree, b->V1.y, v2.y)
                };
            }
            DrawLineEx(b->V1, v2, b->Width, b->Color);
        }
        for (int j = 0; j < tree->LeafCount; j++) {
            TreeLeaf *l = &tree->Leaves[j];
            if ((int)l->Row < i && !(i == tree->CurrentRow && tree->GrowTimer > 0)) {
                DrawCircleV(l->V1, l->Radius, l->Color);
                DrawCircleV(l->V2, l->Radius, l->Color);
            }
        }
    }
}

Tree TreeNewTree() {
    Tree tree = {
        .LeafChance = 0.5,
        .MaxRow = 12,
        .CurrentRow = 0,
        .X = 400,
        .Y = 500,
        .Width = 10,
        .Height = 40,
        .RandomRow = false,
        .SplitChance = 50,
        .SplitAngle = {20, 30},
        .CsBranch = {125, 178, 122, 160, 76, 90},
        .CsLeaf = {150, 204, 190, 230, 159, 178},
        .LeftX = 9999999,
        .RightX = -9999999,
        .GrowTimer = 0,
        .GrowTime = 20,
        .LeafCount = 0
    };
    for (int i = 0; i < MAX_ROWS; i++) {
        tree.BranchCount[i] = 0;
    }
    return tree;
}

#endif // TREE_IMPL
#endif // TREE_H