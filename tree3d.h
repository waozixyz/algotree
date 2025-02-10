#ifndef TREE_3D_H
#define TREE_3D_H

#include <raylib.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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
    int DegX; // Angle around the X-axis (horizontal rotation)
    int DegY; // Angle around the Y-axis (vertical rotation)
    Vector3 V1;
    Vector3 V2;
    float Width;
    float Height;
    Color Color;
} TreeBranch3D;

typedef struct {
    size_t Row;
    Vector3 Position;
    float Radius;
    Color Color;
} TreeLeaf3D;

typedef struct {
    TreeBranch3D Branches[MAX_ROWS][MAX_BRANCHES_PER_ROW];
    int BranchCount[MAX_ROWS];
    TreeLeaf3D Leaves[MAX_LEAVES];
    int LeafCount;
    float LeafChance;
    int MaxRow;
    float X;
    float Y;
    float Z;
    int CurrentRow;
    bool RandomRow;
    int SplitChance;
    int SplitAngle[2]; // Range for random split angles
    unsigned char CsBranch[6]; // RGB color range for branches
    unsigned char CsLeaf[6];   // RGB color range for leaves
    float LeftX;
    float RightX;
    float FrontZ;
    float BackZ;
    int GrowTimer;
    int GrowTime;
    float Width;
    float Height;
} Tree3D;

// Function Declarations
Tree3D TreeNewTree3D();
void TreeLoad3D(Tree3D *tree);
void TreeUpdate3D(Tree3D *tree);
void TreeDraw3D(Tree3D *tree);

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

Vector3 TreeGetRot(int degX, int degY, float height) {
    float radX = degX * DEG_TO_RAD;
    float radY = degY * DEG_TO_RAD;
    return (Vector3){
        .x = sinf(radY) * cosf(radX) * height,
        .y = cosf(radY) * height,
        .z = sinf(radY) * sinf(radX) * height
    };
}

void TreeAppendRow3D(Tree3D *tree) {
    if (tree->CurrentRow + 1 < MAX_ROWS) {
        tree->BranchCount[tree->CurrentRow + 1] = 0;
    } else {
        fprintf(stderr, "Error: Maximum rows exceeded.\n");
    }
}

void TreeAppendBranch3D(Tree3D *tree, int row, TreeBranch3D branch) {
    if (row >= 0 && row < MAX_ROWS && tree->BranchCount[row] < MAX_BRANCHES_PER_ROW) {
        tree->Branches[row][tree->BranchCount[row]++] = branch;
    } else {
        fprintf(stderr, "Error: Maximum branches per row exceeded.\n");
    }
}

void TreeAppendLeaf3D(Tree3D *tree, TreeLeaf3D leaf) {
    if (tree->LeafCount < MAX_LEAVES) {
        tree->Leaves[tree->LeafCount++] = leaf;
    } else {
        fprintf(stderr, "Error: Maximum leaves exceeded.\n");
    }
}

int TreeGetAngle(Tree3D *tree) {
    return rand() % (tree->SplitAngle[1] - tree->SplitAngle[0] + 1) + tree->SplitAngle[0];
}

void TreeAddBranch3D(Tree3D *tree, int degX, int degY, TreeBranch3D *branch) {
    float w = branch->Width * 0.9f;
    float h = branch->Height * 0.95f;
    Vector3 nextPos = {
        .x = branch->V2.x + TreeGetRot(degX, degY, h).x,
        .y = branch->V2.y + TreeGetRot(degX, degY, h).y,
        .z = branch->V2.z + TreeGetRot(degX, degY, h).z
    };
    Color c = TreeGetColor(tree->CsBranch);
    TreeBranch3D newBranch = {
        .DegX = degX,
        .DegY = degY,
        .V1 = branch->V2,
        .V2 = nextPos,
        .Width = w,
        .Height = h,
        .Color = c
    };
    TreeAppendBranch3D(tree, tree->CurrentRow + 1, newBranch);

    float leafChance = ((float)rand() / RAND_MAX) * tree->CurrentRow / tree->MaxRow;
    if (leafChance > tree->LeafChance) {
        TreeLeaf3D newLeaf = {
            .Row = tree->CurrentRow,
            .Position = nextPos,
            .Radius = w,
            .Color = TreeGetColor(tree->CsLeaf)
        };
        TreeAppendLeaf3D(tree, newLeaf);
    }

    if (nextPos.x < tree->LeftX) tree->LeftX = nextPos.x;
    if (nextPos.x > tree->RightX) tree->RightX = nextPos.x;
    if (nextPos.z < tree->FrontZ) tree->FrontZ = nextPos.z;
    if (nextPos.z > tree->BackZ) tree->BackZ = nextPos.z;
}

void TreeGrow3D(Tree3D *tree) {
    TreeAppendRow3D(tree);
    int prevRow = tree->CurrentRow;
    for (int i = 0; i < tree->BranchCount[prevRow]; i++) {
        TreeBranch3D *b = &tree->Branches[prevRow][i];
        int split = rand() % 100;
        if (tree->SplitChance > split) {
            TreeAddBranch3D(tree, b->DegX - TreeGetAngle(tree), b->DegY, b);
            TreeAddBranch3D(tree, b->DegX + TreeGetAngle(tree), b->DegY, b);
        } else {
            TreeAddBranch3D(tree, b->DegX, b->DegY, b);
        }
    }
    tree->CurrentRow++;
}

void TreeLoad3D(Tree3D *tree) {
    int angleX = -90;
    int angleY = 0;
    TreeAppendRow3D(tree);
    TreeBranch3D initialBranch = {
        .DegX = angleX,
        .DegY = angleY,
        .V1 = (Vector3){tree->X, tree->Y, tree->Z},
        .V2 = (Vector3){tree->X, tree->Y, tree->Z},
        .Width = tree->Width,
        .Height = tree->Height,
        .Color = WHITE
    };
    TreeAppendBranch3D(tree, 0, initialBranch);
    tree->GrowTimer = rand() % tree->GrowTime;
    if (tree->RandomRow) {
        int growToRow = rand() % tree->MaxRow;
        while (tree->CurrentRow < growToRow) {
            TreeGrow3D(tree);
        }
    }
}

void TreeUpdate3D(Tree3D *tree) {
    if (tree->GrowTimer > 0) tree->GrowTimer--;
    if (tree->GrowTimer == 0 && tree->CurrentRow < tree->MaxRow) {
        TreeGrow3D(tree);
        tree->GrowTimer = tree->GrowTime;
    }
}
void TreeDraw3D(Tree3D *tree) {
    for (int i = 0; i <= tree->CurrentRow; i++) {
        for (int j = 0; j < tree->BranchCount[i]; j++) {
            TreeBranch3D *b = &tree->Branches[i][j];
            printf("Drawing branch at (%f, %f, %f) -> (%f, %f, %f)\n",
                   b->V1.x, b->V1.y, b->V1.z, b->V2.x, b->V2.y, b->V2.z);
            DrawCylinderEx(b->V1, b->V2, b->Width, b->Width, 8, b->Color);
        }
    }
    for (int j = 0; j < tree->LeafCount; j++) {
        TreeLeaf3D *l = &tree->Leaves[j];
        printf("Drawing leaf at (%f, %f, %f)\n", l->Position.x, l->Position.y, l->Position.z);
        DrawSphere(l->Position, l->Radius, l->Color);
    }
}

Tree3D TreeNewTree3D() {
    Tree3D tree = {
        .LeafChance = 0.5,
        .MaxRow = 12,
        .CurrentRow = 0,
        .X = 0,
        .Y = 0,
        .Z = 0,
        .Width = 1.0f,
        .Height = 4.0f,
        .RandomRow = false,
        .SplitChance = 50,
        .SplitAngle = {20, 30},
        .CsBranch = {125, 178, 122, 160, 76, 90},
        .CsLeaf = {150, 204, 190, 230, 159, 178},
        .LeftX = 9999999,
        .RightX = -9999999,
        .FrontZ = 9999999,
        .BackZ = -9999999,
        .GrowTimer = 0,
        .GrowTime = 20,
        .LeafCount = 0
    };

    printf("Initializing Tree3D struct...\n");
    printf("MaxRows: %d, MaxBranchesPerRow: %d, MaxLeaves: %d\n", MAX_ROWS, MAX_BRANCHES_PER_ROW, MAX_LEAVES);

    for (int i = 0; i < MAX_ROWS; i++) {
        tree.BranchCount[i] = 0;
        printf("BranchCount[%d] initialized to %d\n", i, tree.BranchCount[i]);
    }

    return tree;
}


#endif // TREE_IMPL
#endif // TREE_3D_H