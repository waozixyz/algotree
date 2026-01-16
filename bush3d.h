#ifndef BUSH3D_H
#define BUSH3D_H

#include <raylib.h>
#include <raymath.h>
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

    // Track active burning state for visual effects
    float lastBurnTime;        // Last time this bush received burn damage
    bool isActivelyBurning;    // true if recently receiving fire (for red/orange visuals)

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
void Bush3DDraw(Bush3D* bush, Vector3 playerPos);
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
    // Get base scale from growth
    float growthScale;
    if (bush->CurrentGrowTime >= bush->GrowTime) {
        growthScale = bush->Scale;
    } else {
        // Scale grows from 0.1 to 1.0 over GrowTime
        float progress = bush->CurrentGrowTime / bush->GrowTime;
        growthScale = 0.1f + (0.9f * progress);
    }

    // Apply burn shrinkage: bush continuously shrinks to oblivion as it burns
    if (bush->BurnLevel > 0.0f) {
        // Squared shrinkage for faster early shrink: (1 - burn)^2
        float shrinkFactor = (1.0f - bush->BurnLevel);
        growthScale *= shrinkFactor * shrinkFactor;
    }

    return growthScale;
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
    bush.lastBurnTime = 0.0f;
    bush.isActivelyBurning = false;

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

    // Clear active burning state after 0.5 seconds of no burn damage
    // This allows bushes to disappear if the fire moves away
    if (bush->isActivelyBurning && bush->lastBurnTime > 0) {
        float timeSinceBurn = (float)GetTime() - bush->lastBurnTime;
        if (timeSinceBurn > 0.5f) {
            bush->isActivelyBurning = false;
        }
    }

    // Bush shrinks continuously as BurnLevel increases (handled in Bush3DGetScale)
    // Bush only disappears when BurnLevel reaches 1.0 (fully shrunk to oblivion)

    // Update bounds based on current scale
    float scale = Bush3DGetScale(bush);
    bush->bounds.min = (Vector3){bush->X - 0.5f * scale, bush->Y, bush->Z - 0.5f * scale};
    bush->bounds.max = (Vector3){bush->X + 0.5f * scale, bush->Y + 1.0f * scale, bush->Z + 0.5f * scale};
}

void Bush3DDraw(Bush3D* bush, Vector3 playerPos) {
    if (!bush || bush->IsBurned) return;

    float scale = Bush3DGetScale(bush);
    bool isActiveBurn = bush->isActivelyBurning;
    float burnLevel = bush->BurnLevel;

    // === Draw branches ===
    Color branchColor = (Color){
        bush->ColorBranch[0],
        bush->ColorBranch[1],
        bush->ColorBranch[2],
        255
    };

    if (burnLevel > 0.0f) {
        if (isActiveBurn) {
            // ACTIVELY BURNING: Brown → RED → ORANGE
            if (burnLevel < 0.3f) {
                // Brown to Red transition
                float t = burnLevel / 0.3f;
                branchColor.r = (unsigned char)(branchColor.r * (1.0f - t) + 200 * t);
                branchColor.g = (unsigned char)(branchColor.g * (1.0f - t * 0.7f));
                branchColor.b = (unsigned char)(branchColor.b * (1.0f - t));
            } else {
                // Red to Bright Orange (fire effect)
                branchColor.r = 255;
                branchColor.g = (unsigned char)(60 + burnLevel * 80);
                branchColor.b = (unsigned char)(burnLevel * 50);
            }
        } else {
            // COOLING: Brown → Dark → Black
            float blacken = burnLevel;
            branchColor.r = (unsigned char)(branchColor.r * (1.0f - blacken * 0.8f) + 30 * blacken);
            branchColor.g = (unsigned char)(branchColor.g * (1.0f - blacken * 0.9f));
            branchColor.b = (unsigned char)(branchColor.b * (1.0f - blacken * 0.9f));
        }
    }

    // Apply flame illumination to branches
    Vector3 branchCenter = {bush->X, bush->Y + 0.5f, bush->Z};
    float branchDist = Vector3Distance(branchCenter, playerPos);
    if (branchDist < 4.0f) {
        float illumination = 1.0f - (branchDist / 4.0f);
        illumination = illumination * illumination;
        branchColor.r = (unsigned char)fminf(255, branchColor.r + illumination * 50);
        branchColor.g = (unsigned char)fminf(255, branchColor.g + illumination * 25);
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

    // === Draw leaves ===
    for (int i = 0; i < bush->LeafCount; i++) {
        Vector3 pos = bush->leaves[i].position;
        float radius = bush->leaves[i].radius * scale;
        Color color = bush->leaves[i].color;

        // Scale from base position
        pos.x = bush->X + (pos.x - bush->X) * scale;
        pos.y = bush->Y + (pos.y - bush->Y) * scale;
        pos.z = bush->Z + (pos.z - bush->Z) * scale;

        if (burnLevel > 0.0f) {
            if (isActiveBurn) {
                // ACTIVELY BURNING: Green → RED → BRIGHT ORANGE
                if (burnLevel < 0.3f) {
                    // Green to Red transition
                    float t = burnLevel / 0.3f;
                    color.r = (unsigned char)(color.r * (1.0f - t) + 255 * t);
                    color.g = (unsigned char)(color.g * (1.0f - t * 0.9f));
                    color.b = (unsigned char)(color.b * (1.0f - t));
                } else {
                    // Bright Orange/Red fire
                    color.r = 255;
                    color.g = (unsigned char)(80 + burnLevel * 100);
                    color.b = (unsigned char)(burnLevel * 60);
                }
            } else {
                // COOLING: Green → Dark Red → BLACK
                float blacken = burnLevel;
                color.r = (unsigned char)(color.r * (1.0f - blacken * 0.5f) + 40 * blacken);
                color.g = (unsigned char)(color.g * (1.0f - blacken * 0.95f));
                color.b = (unsigned char)(color.b * (1.0f - blacken * 0.95f));
            }
        }

        // Apply flame illumination to leaves
        float leafDist = Vector3Distance(pos, playerPos);
        if (leafDist < 4.0f) {
            float illumination = 1.0f - (leafDist / 4.0f);
            illumination = illumination * illumination;
            color.r = (unsigned char)fminf(255, color.r + illumination * 50);
            color.g = (unsigned char)fminf(255, color.g + illumination * 25);
        }

        DrawSphere(pos, radius, color);
    }

    // === Draw berries ===
    if (bush->HasBerries && bush->IsMature) {
        for (int i = 0; i < bush->BerryCount; i++) {
            Vector3 pos = bush->berries[i].position;
            float radius = bush->berries[i].radius * scale;
            Color color = bush->berries[i].color;

            // Scale from base position
            pos.x = bush->X + (pos.x - bush->X) * scale;
            pos.y = bush->Y + (pos.y - bush->Y) * scale;
            pos.z = bush->Z + (pos.z - bush->Z) * scale;

            if (isActiveBurn && burnLevel > 0.0f) {
                // Berries glow bright yellow/orange when burning
                color.r = 255;
                color.g = (unsigned char)(180 + burnLevel * 75);
                color.b = (unsigned char)(burnLevel * 100);
            } else if (burnLevel > 0.0f) {
                // Cooling: berries fade to dark
                float blacken = burnLevel;
                color.r = (unsigned char)(color.r * (1.0f - blacken));
                color.g = (unsigned char)(color.g * (1.0f - blacken));
                color.b = (unsigned char)(color.b * (1.0f - blacken));
            }

            // Apply flame illumination to berries
            float berryDist = Vector3Distance(pos, playerPos);
            if (berryDist < 4.0f) {
                float illumination = 1.0f - (berryDist / 4.0f);
                illumination = illumination * illumination;
                color.r = (unsigned char)fminf(255, color.r + illumination * 50);
                color.g = (unsigned char)fminf(255, color.g + illumination * 25);
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

    // Mark as actively burning when receiving damage
    if (amount > 0.0f) {
        bush->lastBurnTime = (float)GetTime();
        bush->isActivelyBurning = true;
    }

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
