#ifndef TREE3D_H
#define TREE3D_H

#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "raymath.h"
#include <string.h>

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

#define TRIG_TABLE_SIZE 360
#define LOD_LEVELS 3
#define BATCH_SIZE 1000


#ifdef TREE3D_IMPLEMENTATION
#define TREE3D_IMPL
#endif

// Pre-declare structures
typedef struct Tree3DBranch Tree3DBranch;
typedef struct Tree3DLeaf Tree3DLeaf;
typedef struct Tree3DMemoryPool Tree3DMemoryPool;
typedef struct Tree3DBatchData Tree3DBatchData;
typedef struct Tree3DGrowthState Tree3DGrowthState;
typedef struct Tree3D Tree3D;

// Memory Pool
struct Tree3DMemoryPool {
    size_t branchPoolIndex;
    size_t leafPoolIndex;
    Tree3DBranch *branchPool;
    Tree3DLeaf *leafPool;
};

// Optimized Branch Structure
struct Tree3DBranch {
    Vector3 V1;
    Vector3 V2;
    float Width;
    float Height;
    Color Color;
    int DegX;
    int DegZ;
    bool isActive;  // For pool management
};

// Leaf Structure
struct Tree3DLeaf {
    size_t Row;
    Vector3 V1;
    Vector3 V2;
    float Radius;
    Color Color;
    bool isActive;  // For pool management
};

// Batch Rendering Data
struct Tree3DBatchData {
    Vector3 *positions;
    float *widths;
    Color *colors;
    int count;
    int capacity;
};

// Growth State Tracking
struct Tree3DGrowthState {
    bool needsUpdate;
    Vector3 currentPos;
    float currentWidth;
    int lastUpdateTime;
};

// Main Tree Structure
struct Tree3D {
    // Memory management
    Tree3DMemoryPool memPool;
    Tree3DBatchData batchData;
    
    // Core data structures
    Tree3DBranch ***Branches;
    int *BranchCount;
    Tree3DLeaf *Leaves;
    int LeafCount;
    
    // Optimization data
    float *sinTable;
    float *cosTable;
    BoundingBox bounds;
    bool needsBoundsUpdate;
    float lodDistances[LOD_LEVELS];
    int lodLevels[LOD_LEVELS];
    Tree3DGrowthState growthState;
    
    // Tree properties
    float LeafChance;
    int MaxRow;
    float Scale;
    float X, Y, Z;
    int CurrentRow;
    bool RandomRow;
    int SplitChance;
    int SplitAngle[2];
    unsigned char CsBranch[6];
    unsigned char CsLeaf[6];
    float MinX, MaxX, MinZ, MaxZ;
    int GrowTimer;
    int GrowTime;
    float Width;
    float Height;
    
    // Allocation tracking
    int AllocatedRows;
    int AllocatedBranchesPerRow;
    int AllocatedLeaves;
};

// Function Declarations
Tree3D Tree3DNewTree(void);
Tree3D Tree3DNewJungleTree(float x, float y, float z);
void Tree3DLoad(Tree3D *tree);
void Tree3DUpdate(Tree3D *tree);
void Tree3DDraw(Tree3D *tree, Camera3D camera);
void Tree3DFree(Tree3D *tree);

// Optimization Function Declarations
void Tree3DInitMemoryPool(Tree3D *tree);
void Tree3DInitTrigTables(Tree3D *tree);
void Tree3DInitBatchData(Tree3D *tree);
void Tree3DUpdateBounds(Tree3D *tree);
Vector3 Tree3DGetRotation(Tree3D *tree, int degX, int degZ);
bool Tree3DIsVisible(const Tree3D *tree, Vector3 point, Camera3D camera);
int Tree3DGetLODLevel(const Tree3D *tree, Vector3 position, Camera3D camera);
void Tree3DBatchDraw(Tree3D *tree,  Camera3D camera);

#ifdef TREE3D_IMPL

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG_TO_RAD (M_PI / 180.0)

// Initialize trig lookup tables
void Tree3DInitTrigTables(Tree3D *tree) {
    tree->sinTable = (float*)malloc(TRIG_TABLE_SIZE * sizeof(float));
    tree->cosTable = (float*)malloc(TRIG_TABLE_SIZE * sizeof(float));
    
    if (!tree->sinTable || !tree->cosTable) {
        fprintf(stderr, "Failed to allocate trig tables\n");
        exit(1);
    }
    
    for (int i = 0; i < TRIG_TABLE_SIZE; i++) {
        float rad = i * DEG_TO_RAD;
        tree->sinTable[i] = sinf(rad);
        tree->cosTable[i] = cosf(rad);
    }
}

void Tree3DInitMemoryPool(Tree3D *tree) {
    tree->memPool.branchPool = (Tree3DBranch*)calloc(
        MAX_ROWS * MAX_BRANCHES_PER_ROW, sizeof(Tree3DBranch));
    tree->memPool.leafPool = (Tree3DLeaf*)calloc(
        MAX_LEAVES, sizeof(Tree3DLeaf));
    
    if (!tree->memPool.branchPool || !tree->memPool.leafPool) {
        fprintf(stderr, "Failed to allocate memory pools\n");
        exit(1);
    }
    
    tree->memPool.branchPoolIndex = 0;
    tree->memPool.leafPoolIndex = 0;
}

void Tree3DInitBatchData(Tree3D *tree) {
    tree->batchData.capacity = BATCH_SIZE;
    tree->batchData.positions = (Vector3*)malloc(BATCH_SIZE * 2 * sizeof(Vector3));
    tree->batchData.widths = (float*)malloc(BATCH_SIZE * 2 * sizeof(float));
    tree->batchData.colors = (Color*)malloc(BATCH_SIZE * sizeof(Color));
    
    if (!tree->batchData.positions || !tree->batchData.widths || !tree->batchData.colors) {
        fprintf(stderr, "Failed to allocate batch data\n");
        exit(1);
    }
    
    tree->batchData.count = 0;
}

// Optimized color generation
Color Tree3DGetColor(unsigned char cs[6]) {
    static unsigned int lastSeed = 0;
    unsigned int currentSeed = time(NULL);
    
    if (currentSeed != lastSeed) {
        srand(currentSeed);
        lastSeed = currentSeed;
    }
    
    return (Color){
        rand() % (cs[1] - cs[0] + 1) + cs[0],
        rand() % (cs[3] - cs[2] + 1) + cs[2],
        rand() % (cs[5] - cs[4] + 1) + cs[4],
        255
    };
}

// Optimized rotation calculation using lookup tables
Vector3 Tree3DGetRotation(Tree3D *tree, int degX, int degZ) {
    int indexX = ((degX % 360) + 360) % 360;
    int indexZ = ((degZ % 360) + 360) % 360;
    
    return (Vector3){
        tree->cosTable[indexZ] * tree->sinTable[indexX],  // X component
        tree->cosTable[indexX],                           // Y component
        tree->sinTable[indexZ]                            // Z component
    };
}
// Frustum culling check
bool Tree3DIsVisible(const Tree3D *tree, Vector3 point, Camera3D camera) {
    // Simple distance-based culling
    float distSq = Vector3DistanceSqr(camera.position, point);
    if (distSq > 10000.0f) return false;
    
    // Simplified bounding check for now
    return (point.x >= tree->bounds.min.x && point.x <= tree->bounds.max.x &&
            point.y >= tree->bounds.min.y && point.y <= tree->bounds.max.y &&
            point.z >= tree->bounds.min.z && point.z <= tree->bounds.max.z);
}
int Tree3DGetLODLevel(const Tree3D *tree, Vector3 position, Camera3D camera) {
    float distance = Vector3Distance(camera.position, position);
    
    for (int i = 0; i < LOD_LEVELS; i++) {
        if (distance <= tree->lodDistances[i]) {
            return tree->lodLevels[i];
        }
    }
    
    return tree->lodLevels[LOD_LEVELS - 1];
}

// Branch and Growth Functions
void Tree3DAppendRow(Tree3D *tree) {
    if (tree->CurrentRow + 1 < tree->AllocatedRows) {
        tree->BranchCount[tree->CurrentRow + 1] = 0;
    }
}

Tree3DBranch* Tree3DGetNextBranch(Tree3D *tree) {
    if (!tree || !tree->memPool.branchPool) {
        return NULL;
    }
    
    if (tree->memPool.branchPoolIndex >= MAX_ROWS * MAX_BRANCHES_PER_ROW) {
        fprintf(stderr, "Branch pool exhausted\n");
        return NULL;
    }
    
    Tree3DBranch* branch = &tree->memPool.branchPool[tree->memPool.branchPoolIndex++];
    return branch;
}


Tree3DLeaf* Tree3DGetNextLeaf(Tree3D *tree) {
    if (tree->memPool.leafPoolIndex >= MAX_LEAVES) {
        return NULL;
    }
    return &tree->memPool.leafPool[tree->memPool.leafPoolIndex++];
}


void Tree3DAppendBranch(Tree3D *tree, int row, Tree3DBranch branch) {
    if (!tree || !tree->Branches || !tree->BranchCount) {
        fprintf(stderr, "Tree not properly initialized\n");
        return;
    }
    
    if (row >= MAX_ROWS || row < 0) {
        fprintf(stderr, "Invalid row index: %d\n", row);
        return;
    }
    
    if (tree->BranchCount[row] >= MAX_BRANCHES_PER_ROW) {
        fprintf(stderr, "Too many branches in row %d\n", row);
        return;
    }
    
    Tree3DBranch* newBranch = Tree3DGetNextBranch(tree);
    if (!newBranch) {
        fprintf(stderr, "Failed to get new branch\n");
        return;
    }
    
    *newBranch = branch;
    newBranch->isActive = true;
    tree->Branches[row][tree->BranchCount[row]] = newBranch;
    tree->BranchCount[row]++;
    tree->needsBoundsUpdate = true;
}

void Tree3DAppendLeaf(Tree3D *tree, Tree3DLeaf leaf) {
    if (tree->LeafCount >= MAX_LEAVES) {
        return;
    }
    
    Tree3DLeaf* newLeaf = Tree3DGetNextLeaf(tree);
    if (!newLeaf) return;
    
    *newLeaf = leaf;
    newLeaf->isActive = true;
    tree->LeafCount++;
}

void Tree3DAddBranch(Tree3D *tree, int degX, int degZ, Tree3DBranch *branch) {
    float w = branch->Width * 0.9f;
    float h = branch->Height * 0.95f;
    Vector3 pos = branch->V2;
    Vector3 rot = Tree3DGetRotation(tree, degX, degZ);
    
    Vector3 newPos = {
        pos.x + rot.x * h * tree->Scale,
        pos.y + rot.y * h * tree->Scale,
        pos.z + rot.z * h * tree->Scale
    };
    
    Tree3DBranch newBranch = {
        .V1 = pos,
        .V2 = newPos,
        .Width = w,
        .Height = h,
        .Color = Tree3DGetColor(tree->CsBranch),
        .DegX = degX,
        .DegZ = degZ,
        .isActive = true
    };

    Tree3DAppendBranch(tree, tree->CurrentRow + 1, newBranch);

    // Leaf generation with optimized random check
    float leafChance = ((float)rand() / RAND_MAX) * tree->CurrentRow / tree->MaxRow;
    if (leafChance > tree->LeafChance) {
        Vector3 rotLeaf = Tree3DGetRotation(tree, degX * 2, degZ * 2);
        Vector3 leafOffset = {
            rotLeaf.x * w,
            rotLeaf.y * w,
            rotLeaf.z * w
        };
        
        Tree3DLeaf newLeaf = {
            .Row = tree->CurrentRow,
            .Radius = w,
            .V1 = {
                newPos.x + leafOffset.x,
                newPos.y + leafOffset.y,
                newPos.z + leafOffset.z
            },
            .V2 = {
                newPos.x - leafOffset.x,
                newPos.y - leafOffset.y,
                newPos.z - leafOffset.z
            },
            .Color = Tree3DGetColor(tree->CsLeaf),
            .isActive = true
        };
        
        Tree3DAppendLeaf(tree, newLeaf);
    }

    // Update bounds
    if (newPos.x < tree->MinX) tree->MinX = newPos.x;
    if (newPos.x > tree->MaxX) tree->MaxX = newPos.x + w;
    if (newPos.z < tree->MinZ) tree->MinZ = newPos.z;
    if (newPos.z > tree->MaxZ) tree->MaxZ = newPos.z + w;
}

void Tree3DUpdateBounds(Tree3D *tree) {
    if (!tree->needsBoundsUpdate) return;
    
    tree->bounds = (BoundingBox){
        .min = {tree->MinX, tree->Y, tree->MinZ},
        .max = {tree->MaxX, tree->Y + (tree->Height * tree->Scale * tree->CurrentRow), tree->MaxZ}
    };
    
    tree->needsBoundsUpdate = false;
}

float Tree3DGetNextPos(Tree3D *tree, float a, float b) {
    return b + (a - b) * tree->GrowTimer / (float)tree->GrowTime;
}
void Tree3DGrow(Tree3D *tree) {
    if (tree->CurrentRow >= tree->MaxRow) return;

    Tree3DAppendRow(tree);
    int prevRow = tree->CurrentRow;
    for (int i = 0; i < tree->BranchCount[prevRow]; i++) {
        Tree3DBranch *b = tree->Branches[prevRow][i];
        if (!b || !b->isActive) continue;

        int split = rand() % 100;
        if (tree->SplitChance > split) {
            // Two branches with optimized angle calculation
            int angleX1 = tree->SplitAngle[0] + (rand() % (tree->SplitAngle[1] - tree->SplitAngle[0]));
            int angleZ1 = tree->SplitAngle[0] + (rand() % (tree->SplitAngle[1] - tree->SplitAngle[0]));
            
            // Randomly choose split direction (X, Z, or both)
            int splitType = rand() % 3;  // 0 = X, 1 = Z, 2 = both
            
            // First branch is always the same
            Tree3DAddBranch(tree, b->DegX + angleX1, b->DegZ + angleZ1, b);
            
            // Second branch mirrors based on split type
            switch(splitType) {
                case 0:  // Mirror X only
                    Tree3DAddBranch(tree, b->DegX - angleX1, b->DegZ + angleZ1, b);
                    break;
                case 1:  // Mirror Z only
                    Tree3DAddBranch(tree, b->DegX + angleX1, b->DegZ - angleZ1, b);
                    break;
                case 2:  // Mirror both
                    Tree3DAddBranch(tree, b->DegX - angleX1, b->DegZ - angleZ1, b);
                    break;
            }
        } else {
            // Single branch with slight variation
            int angleX = (rand() % 21) - 10;
            int angleZ = (rand() % 21) - 10;
            Tree3DAddBranch(tree, b->DegX + angleX, b->DegZ + angleZ, b);
        }
    }
        
    tree->CurrentRow++;
    tree->needsBoundsUpdate = true;
}

void Tree3DLoad(Tree3D *tree) {
    if (!tree || !tree->Branches || !tree->BranchCount) {
        fprintf(stderr, "Tree not properly initialized\n");
        return;
    }

    tree->CurrentRow = 0;
    tree->memPool.branchPoolIndex = 0;
    tree->memPool.leafPoolIndex = 0;
    tree->LeafCount = 0;
    
    memset(tree->BranchCount, 0, MAX_ROWS * sizeof(int));
    
    Tree3DBranch initialBranch = {
        .V1 = {tree->X, tree->Y, tree->Z},
        .V2 = {tree->X, tree->Y + (tree->Height * tree->Scale), tree->Z},
        .Width = tree->Width * tree->Scale,
        .Height = tree->Height * tree->Scale,
        .Color = Tree3DGetColor(tree->CsBranch),
        .DegX = 0,
        .DegZ = 0,
        .isActive = true
    };

    Tree3DAppendBranch(tree, 0, initialBranch);
    tree->GrowTimer = tree->GrowTime;
    
    if (tree->RandomRow) {
        int targetRow = rand() % tree->MaxRow;
        while (tree->CurrentRow < targetRow) {
            Tree3DGrow(tree);
        }
    }
    
    tree->needsBoundsUpdate = true;
}

void Tree3DUpdate(Tree3D *tree) {
    if (tree->CurrentRow >= tree->MaxRow) return;

    if (tree->GrowTimer > 0) {
        tree->GrowTimer--;
        tree->growthState.needsUpdate = true;
    }

    if (tree->GrowTimer == 0) {
        Tree3DGrow(tree);
        tree->GrowTimer = tree->GrowTime;
    }
}

void Tree3DBatchDraw(Tree3D *tree, Camera3D camera) {
    tree->batchData.count = 0;
    
    for (int i = 0; i <= tree->CurrentRow; i++) {
        for (int j = 0; j < tree->BranchCount[i]; j++) {
            Tree3DBranch *b = tree->Branches[i][j];
            if (!b || !b->isActive) continue;
            
            Vector3 v2 = b->V2;
            if (i == tree->CurrentRow && tree->GrowTimer > 0) {
                v2 = (Vector3){
                    Tree3DGetNextPos(tree, b->V1.x, v2.x),
                    Tree3DGetNextPos(tree, b->V1.y, v2.y),
                    Tree3DGetNextPos(tree, b->V1.z, v2.z)
                };
            }
            
            if (!Tree3DIsVisible(tree, b->V1, camera) && !Tree3DIsVisible(tree, v2, camera)) {
                continue;
            }
            
            int lodLevel = Tree3DGetLODLevel(tree, b->V1, camera);
            
            tree->batchData.positions[tree->batchData.count * 2] = b->V1;
            tree->batchData.positions[tree->batchData.count * 2 + 1] = v2;
            tree->batchData.widths[tree->batchData.count * 2] = b->Width;
            tree->batchData.widths[tree->batchData.count * 2 + 1] = b->Width * 0.8f;
            tree->batchData.colors[tree->batchData.count] = b->Color;
            tree->batchData.count++;
            
            if (tree->batchData.count >= BATCH_SIZE) {
                // Draw batch using raylib's batch drawing
                for (int k = 0; k < tree->batchData.count; k++) {
                    DrawCylinderEx(
                        tree->batchData.positions[k * 2],
                        tree->batchData.positions[k * 2 + 1],
                        tree->batchData.widths[k * 2],
                        tree->batchData.widths[k * 2 + 1],
                        lodLevel,
                        tree->batchData.colors[k]
                    );
                }
                tree->batchData.count = 0;
            }
        }
    }
    
    // Draw remaining batch
    if (tree->batchData.count > 0) {
        for (int k = 0; k < tree->batchData.count; k++) {
            DrawCylinderEx(
                tree->batchData.positions[k * 2],
                tree->batchData.positions[k * 2 + 1],
                tree->batchData.widths[k * 2],
                tree->batchData.widths[k * 2 + 1],
                8,
                tree->batchData.colors[k]
            );
        }
    }
    
    // Draw leaves
    for (int i = 0; i < tree->LeafCount; i++) {
        Tree3DLeaf *l = &tree->memPool.leafPool[i];
        if (!l->isActive) continue;
        
        if ((int)l->Row < tree->CurrentRow && 
            !(i == tree->CurrentRow && tree->GrowTimer > 0)) {
            if (Tree3DIsVisible(tree, l->V1, camera)) {
                DrawSphere(l->V1, l->Radius * tree->Scale, l->Color);
            }
            if (Tree3DIsVisible(tree, l->V2, camera)) {
                DrawSphere(l->V2, l->Radius * tree->Scale, l->Color);
            }
        }
    }
}
void Tree3DDraw(Tree3D *tree, Camera3D camera) {
    if (tree->needsBoundsUpdate) {
        Tree3DUpdateBounds(tree);
    }
    Tree3DBatchDraw(tree, camera);
}
Tree3D Tree3DNewTree() {
    Tree3D tree = {0};
    
    // Allocate BranchCount array
    tree.BranchCount = (int*)malloc(MAX_ROWS * sizeof(int));
    if (!tree.BranchCount) {
        fprintf(stderr, "Failed to allocate BranchCount array\n");
        exit(1);
    }
    memset(tree.BranchCount, 0, MAX_ROWS * sizeof(int));
    
    // Allocate three levels of pointers
    tree.Branches = (Tree3DBranch***)malloc(MAX_ROWS * sizeof(Tree3DBranch**));
    if (!tree.Branches) {
        fprintf(stderr, "Failed to allocate Branches array\n");
        free(tree.BranchCount);
        exit(1);
    }
    
    for (int i = 0; i < MAX_ROWS; i++) {
        tree.Branches[i] = (Tree3DBranch**)malloc(MAX_BRANCHES_PER_ROW * sizeof(Tree3DBranch*));
        if (!tree.Branches[i]) {
            for (int j = 0; j < i; j++) {
                free(tree.Branches[j]);
            }
            free(tree.Branches);
            free(tree.BranchCount);
            fprintf(stderr, "Failed to allocate row %d\n", i);
            exit(1);
        }
        for (int j = 0; j < MAX_BRANCHES_PER_ROW; j++) {
            tree.Branches[i][j] = NULL;
        }
    }
    
    Tree3DInitMemoryPool(&tree);
    Tree3DInitTrigTables(&tree);
    Tree3DInitBatchData(&tree);
    
    tree.AllocatedRows = MAX_ROWS;
    tree.AllocatedBranchesPerRow = MAX_BRANCHES_PER_ROW;
    tree.AllocatedLeaves = MAX_LEAVES;
    
    tree.LeafChance = 0.5f;
    tree.MaxRow = 10;
    tree.Scale = 1.0f;
    tree.Width = 1.0f;
    tree.Height = 4.0f;
    tree.SplitChance = 50;
    tree.SplitAngle[0] = 20;
    tree.SplitAngle[1] = 30;
    
    tree.CsBranch[0] = 125; tree.CsBranch[1] = 178;
    tree.CsBranch[2] = 122; tree.CsBranch[3] = 160;
    tree.CsBranch[4] = 76;  tree.CsBranch[5] = 90;
    
    tree.CsLeaf[0] = 150; tree.CsLeaf[1] = 204;
    tree.CsLeaf[2] = 190; tree.CsLeaf[3] = 230;
    tree.CsLeaf[4] = 159; tree.CsLeaf[5] = 178;
    
    tree.lodDistances[0] = 10.0f;
    tree.lodDistances[1] = 30.0f;
    tree.lodDistances[2] = 60.0f;
    
    tree.lodLevels[0] = 8;
    tree.lodLevels[1] = 6;
    tree.lodLevels[2] = 4;
    
    tree.MinX = 9999999.0f;
    tree.MaxX = -9999999.0f;
    tree.MinZ = 9999999.0f;
    tree.MaxZ = -9999999.0f;
    
    tree.GrowTimer = 0;
    tree.GrowTime = 20;
    
    return tree;
}

// Jungle tree variant - darker colors, taller, denser foliage
Tree3D Tree3DNewJungleTree(float x, float y, float z) {
    Tree3D tree = Tree3DNewTree();

    tree.X = x;
    tree.Y = y;
    tree.Z = z;

    // Jungle properties: taller, denser
    tree.Height = 6.0f;       // Taller than normal (4.0f)
    tree.Width = 0.9f;        // Slightly thinner
    tree.MaxRow = 14;         // More rows for dense canopy
    tree.LeafChance = 0.65f;  // More leaves

    // Darker jungle colors
    // Branches: Dark brown/wet wood (60-90, 40-60, 30-50)
    tree.CsBranch[0] = 60;  tree.CsBranch[1] = 90;
    tree.CsBranch[2] = 40;  tree.CsBranch[3] = 60;
    tree.CsBranch[4] = 30;  tree.CsBranch[5] = 50;

    // Leaves: Dark jungle green (30-60, 100-140, 30-60)
    tree.CsLeaf[0] = 30;   tree.CsLeaf[1] = 60;
    tree.CsLeaf[2] = 100;  tree.CsLeaf[3] = 140;
    tree.CsLeaf[4] = 30;   tree.CsLeaf[5] = 60;

    return tree;
}


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
    
    if (tree->memPool.branchPool) {
        free(tree->memPool.branchPool);
        tree->memPool.branchPool = NULL;
    }
    
    if (tree->memPool.leafPool) {
        free(tree->memPool.leafPool);
        tree->memPool.leafPool = NULL;
    }
    
    if (tree->sinTable) {
        free(tree->sinTable);
        tree->sinTable = NULL;
    }
    
    if (tree->cosTable) {
        free(tree->cosTable);
        tree->cosTable = NULL;
    }
    
    if (tree->batchData.positions) {
        free(tree->batchData.positions);
        tree->batchData.positions = NULL;
    }
    
    if (tree->batchData.widths) {
        free(tree->batchData.widths);
        tree->batchData.widths = NULL;
    }
    
    if (tree->batchData.colors) {
        free(tree->batchData.colors);
        tree->batchData.colors = NULL;
    }
}


#endif // TREE3D_IMPL
#endif // TREE3D_H
