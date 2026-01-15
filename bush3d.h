#ifndef BUSH3D_H
#define BUSH3D_H

#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Bush configuration
#ifndef BUSH_MAX_BRANCHES
#define BUSH_MAX_BRANCHES 24
#endif

#ifndef BUSH_MAX_LEAVES
#define BUSH_MAX_LEAVES 100
#endif

// Pre-declare structure
typedef struct Bush3D Bush3D;

// Individual leaf
typedef struct {
    Vector3 position;
    float radius;
    Color color;
} BushLeaf;

// Individual berry (edible indicator)
typedef struct {
    Vector3 position;
    float radius;
    Color color;
} BushBerry;

// Main Bush Structure
struct Bush3D {
    // Position and scale
    float X, Y, Z;
    float Scale;

    // Growth
    int MaxBranches;
    float GrowTime;        // Time to full maturity (seconds)
    float CurrentGrowTime;
    bool IsMature;

    // Visual properties
    unsigned char ColorLeafMin[3];
    unsigned char ColorLeafMax[3];
    unsigned char ColorBranch[3];
    int LeafCount;
    float LeafSize;

    // Berries (edible indicator - appear when mature)
    bool HasBerries;
    int BerryCount;
    float BerrySize;
    unsigned char ColorBerry[3];

    // Burn state
    float BurnLevel;      // 0.0 = fine, 1.0 = burned
    bool IsBurned;

    // Branch data for structure
    struct {
        Vector3 start;
        Vector3 end;
        float width;
    } branches[BUSH_MAX_BRANCHES];
    int branchCount;

    // Leaf data
    BushLeaf leaves[BUSH_MAX_LEAVES];

    // Berry data
    BushBerry berries[BUSH_MAX_LEAVES];  // Berries use same max as leaves

    // Bounding box for collision
    BoundingBox bounds;

    // Random seed for this bush
    unsigned int seed;
};

// Function Declarations
Bush3D Bush3DNewBush(float x, float y, float z);
void Bush3DLoad(Bush3D* bush);
void Bush3DUpdate(Bush3D* bush, float deltaTime);
void Bush3DDraw(Bush3D* bush);
bool Bush3DIsMature(const Bush3D* bush);
void Bush3DBurn(Bush3D* bush, float amount);
BoundingBox Bush3DGetBounds(Bush3D* bush);
void Bush3DFree(Bush3D* bush);

// Helper: Get current scale based on growth
float Bush3DGetScale(const Bush3D* bush);

// Helper: Random float 0-1
float Bush3DRandom(Bush3D* bush);

#ifdef BUSH3D_IMPLEMENTATION

float Bush3DRandom(Bush3D* bush) {
    bush->seed = bush->seed * 1103515245 + 12345;
    return (float)(bush->seed & 0x7FFF) / (float)0x7FFF;
}

float Bush3DGetScale(const Bush3D* bush) {
    if (bush->CurrentGrowTime >= bush->GrowTime) {
        return bush->Scale;
    }
    // Scale grows from 0.1 to 1.0 over GrowTime
    float progress = bush->CurrentGrowTime / bush->GrowTime;
    return 0.1f + (0.9f * progress);
}

Bush3D Bush3DNewBush(float x, float y, float z) {
    Bush3D bush = {0};

    // Position
    bush.X = x;
    bush.Y = y;
    bush.Z = z;
    bush.Scale = 1.0f;

    // Growth settings
    bush.MaxBranches = 8 + (int)(Bush3DRandom(&bush) * 8);  // 8-16 branches
    bush.GrowTime = 3.0f + Bush3DRandom(&bush) * 2.0f;       // 3-5 seconds to mature
    bush.CurrentGrowTime = 0.0f;
    bush.IsMature = false;

    // Visual properties
    bush.ColorLeafMin[0] = 34;  bush.ColorLeafMin[1] = 139; bush.ColorLeafMin[2] = 34;   // Forest green
    bush.ColorLeafMax[0] = 50;  bush.ColorLeafMax[1] = 205; bush.ColorLeafMax[2] = 50;   // Lime green
    bush.ColorBranch[0] = 101; bush.ColorBranch[1] = 67;  bush.ColorBranch[2] = 33;    // Brown
    bush.LeafCount = 30 + (int)(Bush3DRandom(&bush) * 40);  // 30-70 leaves
    bush.LeafSize = 0.08f + Bush3DRandom(&bush) * 0.06f;    // 0.08-0.14

    // Berry properties (appear when mature)
    bush.HasBerries = false;
    bush.BerryCount = 8 + (int)(Bush3DRandom(&bush) * 12);  // 8-20 berries
    bush.BerrySize = 0.04f;
    bush.ColorBerry[0] = 220; bush.ColorBerry[1] = 20; bush.ColorBerry[2] = 60;  // Red

    // Burn state
    bush.BurnLevel = 0.0f;
    bush.IsBurned = false;

    bush.branchCount = 0;

    // Initialize bounds
    bush.bounds = (BoundingBox){
        .min = (Vector3){x - 0.5f, y, z - 0.5f},
        .max = (Vector3){x + 0.5f, y + 1.0f, z + 0.5f}
    };

    return bush;
}

void Bush3DLoad(Bush3D* bush) {
    if (!bush) return;

    // Generate branches radiating from center
    bush->branchCount = bush->MaxBranches;

    for (int i = 0; i < bush->MaxBranches && i < BUSH_MAX_BRANCHES; i++) {
        // Random angle
        float angle = Bush3DRandom(bush) * 6.28318f;  // 0 to 2pi

        // Branch direction (up and outward)
        float outward = 0.3f + Bush3DRandom(bush) * 0.4f;
        float up = 0.5f + Bush3DRandom(bush) * 0.5f;

        bush->branches[i].start = (Vector3){bush->X, bush->Y, bush->Z};
        bush->branches[i].end = (Vector3){
            bush->X + cosf(angle) * outward,
            bush->Y + up,
            bush->Z + sinf(angle) * outward
        };
        bush->branches[i].width = 0.02f + Bush3DRandom(bush) * 0.03f;
    }

    // Generate leaves
    for (int i = 0; i < bush->LeafCount && i < BUSH_MAX_LEAVES; i++) {
        // Pick a random branch endpoint as base
        int branchIdx = (int)(Bush3DRandom(bush) * bush->branchCount);
        Vector3 base = bush->branches[branchIdx].end;

        // Add some offset
        float offset = 0.1f + Bush3DRandom(bush) * 0.2f;
        float angle = Bush3DRandom(bush) * 6.28318f;

        bush->leaves[i].position = (Vector3){
            base.x + cosf(angle) * offset,
            base.y + Bush3DRandom(bush) * 0.2f,
            base.z + sinf(angle) * offset
        };
        bush->leaves[i].radius = bush->LeafSize;

        // Random green color
        float t = Bush3DRandom(bush);
        bush->leaves[i].color = (Color){
            (unsigned char)(bush->ColorLeafMin[0] + t * (bush->ColorLeafMax[0] - bush->ColorLeafMin[0])),
            (unsigned char)(bush->ColorLeafMin[1] + t * (bush->ColorLeafMax[1] - bush->ColorLeafMin[1])),
            (unsigned char)(bush->ColorLeafMin[2] + t * (bush->ColorLeafMax[2] - bush->ColorLeafMin[2])),
            255
        };
    }

    // Generate berries (initially hidden)
    for (int i = 0; i < bush->BerryCount && i < BUSH_MAX_LEAVES; i++) {
        bush->berries[i].position = (Vector3){
            bush->X + (Bush3DRandom(bush) - 0.5f) * 0.6f,
            bush->Y + 0.3f + Bush3DRandom(bush) * 0.3f,
            bush->Z + (Bush3DRandom(bush) - 0.5f) * 0.6f
        };
        bush->berries[i].radius = bush->BerrySize;
        bush->berries[i].color = (Color){
            bush->ColorBerry[0],
            bush->ColorBerry[1],
            bush->ColorBerry[2],
            255
        };
    }
}

void Bush3DUpdate(Bush3D* bush, float deltaTime) {
    if (!bush) return;

    // Update growth
    if (bush->CurrentGrowTime < bush->GrowTime) {
        bush->CurrentGrowTime += deltaTime;
        if (bush->CurrentGrowTime >= bush->GrowTime) {
            bush->CurrentGrowTime = bush->GrowTime;
            bush->IsMature = true;
            bush->HasBerries = true;  // Berries appear when mature
        }
    }

    // Update bounds based on current scale
    float scale = Bush3DGetScale(bush);
    bush->bounds.min = (Vector3){bush->X - 0.5f * scale, bush->Y, bush->Z - 0.5f * scale};
    bush->bounds.max = (Vector3){bush->X + 0.5f * scale, bush->Y + 1.0f * scale, bush->Z + 0.5f * scale};
}

void Bush3DDraw(Bush3D* bush) {
    if (!bush || bush->IsBurned) return;

    float scale = Bush3DGetScale(bush);

    // Draw branches
    Color branchColor = (Color){
        bush->ColorBranch[0],
        bush->ColorBranch[1],
        bush->ColorBranch[2],
        255
    };

    // Apply burn color if burning
    if (bush->BurnLevel > 0.0f) {
        // Blend toward black/burned color
        float burn = bush->BurnLevel;
        branchColor.r = (unsigned char)(branchColor.r * (1.0f - burn * 0.7f));
        branchColor.g = (unsigned char)(branchColor.g * (1.0f - burn * 0.9f));
        branchColor.b = (unsigned char)(branchColor.b * (1.0f - burn * 0.9f));
    }

    for (int i = 0; i < bush->branchCount; i++) {
        Vector3 start = bush->branches[i].start;
        Vector3 end = bush->branches[i].end;

        // Scale from base position
        end.x = bush->X + (end.x - bush->X) * scale;
        end.y = bush->Y + (end.y - bush->Y) * scale;
        end.z = bush->Z + (end.z - bush->Z) * scale;

        DrawLine3D(start, end, branchColor);
    }

    // Draw leaves
    for (int i = 0; i < bush->LeafCount; i++) {
        Vector3 pos = bush->leaves[i].position;
        float radius = bush->leaves[i].radius * scale;
        Color color = bush->leaves[i].color;

        // Scale from base position
        pos.x = bush->X + (pos.x - bush->X) * scale;
        pos.y = bush->Y + (pos.y - bush->Y) * scale;
        pos.z = bush->Z + (pos.z - bush->Z) * scale;

        // Apply burn color
        if (bush->BurnLevel > 0.0f) {
            float burn = bush->BurnLevel;
            color.r = (unsigned char)(color.r * (1.0f - burn * 0.3f));  // More red when burning
            color.g = (unsigned char)(color.g * (1.0f - burn * 0.8f));
            color.b = (unsigned char)(color.b * (1.0f - burn * 0.8f));
        }

        DrawSphere(pos, radius, color);
    }

    // Draw berries (only when mature)
    if (bush->HasBerries && bush->IsMature) {
        for (int i = 0; i < bush->BerryCount; i++) {
            Vector3 pos = bush->berries[i].position;
            float radius = bush->berries[i].radius * scale;
            Color color = bush->berries[i].color;

            // Scale from base position
            pos.x = bush->X + (pos.x - bush->X) * scale;
            pos.y = bush->Y + (pos.y - bush->Y) * scale;
            pos.z = bush->Z + (pos.z - bush->Z) * scale;

            // Berries glow more when burning (before being destroyed)
            if (bush->BurnLevel > 0.0f) {
                color.r = (unsigned char)fminf(255, color.r + bush->BurnLevel * 50);
            }

            DrawSphere(pos, radius, color);
        }
    }
}

bool Bush3DIsMature(const Bush3D* bush) {
    return bush ? bush->IsMature : false;
}

void Bush3DBurn(Bush3D* bush, float amount) {
    if (!bush) return;
    bush->BurnLevel += amount;
    if (bush->BurnLevel >= 1.0f) {
        bush->BurnLevel = 1.0f;
        bush->IsBurned = true;
    }
}

BoundingBox Bush3DGetBounds(Bush3D* bush) {
    if (!bush) {
        return (BoundingBox){
            .min = {0}, .max = {0}
        };
    }
    return bush->bounds;
}

void Bush3DFree(Bush3D* bush) {
    if (!bush) return;
    // Bush is stack-allocated, just reset state
    bush->branchCount = 0;
    bush->LeafCount = 0;
    bush->BerryCount = 0;
}

#endif  // BUSH3D_IMPLEMENTATION

#endif  // BUSH3D_H
