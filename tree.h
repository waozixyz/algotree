#ifndef TREE_H
#define TREE_H

#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// Define this macro in ONE source file to include the implementation
#ifdef TREE_IMPLEMENTATION
#define TREE_IMPL
#endif

#define MAX_ROWS 100
#define MAX_BRANCHES_PER_ROW 1000
#define MAX_LEAVES 10000

typedef struct {
    int deg;
    Vector2 v1;
    Vector2 v2;
    float w;
    float h;
    Color color;
} Branch;

typedef struct {
    size_t row;
    Vector2 v1;
    Vector2 v2;
    float r;
    Color color;
} Leaf;

typedef struct {
    Branch branches[MAX_ROWS][MAX_BRANCHES_PER_ROW];
    int branch_count[MAX_ROWS];
    Leaf leaves[MAX_LEAVES];
    int leaf_count;
    float leaf_chance;
    int max_row;
    float x;
    float y;
    int current_row;
    bool random_row;
    int split_chance;
    int split_angle[2];
    unsigned char cs_branch[6];
    unsigned char cs_leaf[6];
    float left_x;
    float right_x;
    int grow_timer;
    int grow_time;
    float w;
    float h;
} Tree;

// Function declarations
Color get_color(unsigned char cs[6]);
void append_row(Tree *tree);
void append_branch(Tree *tree, int row, Branch b);
void append_leaf(Tree *tree, Leaf l);
int get_angle(Tree *tree);
float get_rot_x(int deg);
float get_rot_y(int deg);
void add_branch(Tree *tree, int deg, Branch *b);
float get_next_pos(Tree *tree, float a, float b);
void grow(Tree *tree);
void load(Tree *tree);
void update(Tree *tree);
void draw(Tree *tree);
Tree new_tree();

#ifdef TREE_IMPL

// Define DEG_TO_RAD using M_PI or fallback to manual definition
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG_TO_RAD (M_PI / 180.0)

Color get_color(unsigned char cs[6]) {
    unsigned char r = rand() % (cs[1] - cs[0] + 1) + cs[0];
    unsigned char g = rand() % (cs[3] - cs[2] + 1) + cs[2];
    unsigned char b = rand() % (cs[5] - cs[4] + 1) + cs[4];
    return (Color){r, g, b, 255};
}

void append_row(Tree *tree) {
    tree->branch_count[tree->current_row + 1] = 0;
}

void append_branch(Tree *tree, int row, Branch b) {
    tree->branches[row][tree->branch_count[row]++] = b;
}

void append_leaf(Tree *tree, Leaf l) {
    tree->leaves[tree->leaf_count++] = l;
}

int get_angle(Tree *tree) {
    return rand() % (tree->split_angle[1] - tree->split_angle[0] + 1) + tree->split_angle[0];
}

float get_rot_x(int deg) {
    return cosf(deg * DEG_TO_RAD);
}

float get_rot_y(int deg) {
    return sinf(deg * DEG_TO_RAD);
}

void add_branch(Tree *tree, int deg, Branch *b) {
    float w = b->w * 0.9;
    float h = b->h * 0.95;
    float px = b->v2.x;
    float py = b->v2.y;
    float nx = px + get_rot_x(deg) * h;
    float ny = py + get_rot_y(deg) * h;
    Color c = get_color(tree->cs_branch);

    Branch new_branch = {
        .deg = deg,
        .v1 = (Vector2){px, py},
        .v2 = (Vector2){nx, ny},
        .w = w,
        .h = h,
        .color = c
    };
    append_branch(tree, tree->current_row + 1, new_branch);

    float leaf_chance = ((float)rand() / RAND_MAX) * tree->current_row / tree->max_row;
    if (leaf_chance > tree->leaf_chance) {
        float div_x = get_rot_x(deg * 2) * w;
        float div_y = get_rot_y(deg * 2) * w;
        Leaf new_leaf = {
            .row = tree->current_row,
            .r = w,
            .v1 = (Vector2){nx + div_x, ny + div_y},
            .v2 = (Vector2){nx - div_x, ny - div_y},
            .color = get_color(tree->cs_leaf)
        };
        append_leaf(tree, new_leaf);
    }

    if (nx < tree->left_x) tree->left_x = nx;
    if (nx > tree->right_x) tree->right_x = nx + w;
}

float get_next_pos(Tree *tree, float a, float b) {
    return b + (a - b) * tree->grow_timer / (float)tree->grow_time;
}

void grow(Tree *tree) {
    append_row(tree);
    int prev_row = tree->current_row;
    for (int i = 0; i < tree->branch_count[prev_row]; i++) {
        Branch *b = &tree->branches[prev_row][i];
        int split = rand() % 100;
        if (tree->split_chance > split) {
            add_branch(tree, b->deg - get_angle(tree), b);
            add_branch(tree, b->deg + get_angle(tree), b);
        } else {
            add_branch(tree, b->deg, b);
        }
    }
    tree->current_row++;
}

void load(Tree *tree) {
    int angle = -90;
    append_row(tree);
    Branch initial_branch = {
        .deg = angle,
        .v1 = (Vector2){tree->x, tree->y},
        .v2 = (Vector2){tree->x, tree->y},
        .w = tree->w,
        .h = tree->h,
        .color = WHITE
    };
    append_branch(tree, 0, initial_branch);
    tree->grow_timer = rand() % tree->grow_time;
    if (tree->random_row) {
        int grow_to_row = rand() % tree->max_row;
        while (tree->current_row < grow_to_row) {
            grow(tree);
        }
    }
}

void update(Tree *tree) {
    if (tree->grow_timer > 0) tree->grow_timer--;
    if (tree->grow_timer == 0 && tree->current_row < tree->max_row) {
        grow(tree);
        tree->grow_timer = tree->grow_time;
    }
}

void draw(Tree *tree) {
    for (int i = 0; i <= tree->current_row; i++) {
        for (int j = 0; j < tree->branch_count[i]; j++) {
            Branch *b = &tree->branches[i][j];
            Vector2 v2 = b->v2;
            if (i == tree->current_row && tree->grow_timer > 0) {
                v2 = (Vector2){
                    .x = get_next_pos(tree, b->v1.x, v2.x),
                    .y = get_next_pos(tree, b->v1.y, v2.y)
                };
            }
            DrawLineEx(b->v1, v2, b->w, b->color);
        }
        for (int j = 0; j < tree->leaf_count; j++) {
            Leaf *l = &tree->leaves[j];
            // Fix signed/unsigned comparison warning
            if ((int)l->row < i && !(i == tree->current_row && tree->grow_timer > 0)) {
                DrawCircleV(l->v1, l->r, l->color);
                DrawCircleV(l->v2, l->r, l->color);
            }
        }
    }
}

Tree new_tree() {
    Tree tree = {
        .leaf_chance = 0.5,
        .max_row = 12,
        .current_row = 0,
        .x = 400,
        .y = 500,
        .w = 10,
        .h = 40,
        .random_row = false,
        .split_chance = 50,
        .split_angle = {20, 30},
        .cs_branch = {125, 178, 122, 160, 76, 90},
        .cs_leaf = {150, 204, 190, 230, 159, 178},
        .left_x = 9999999,
        .right_x = -9999999,
        .grow_timer = 0,
        .grow_time = 20,
        .leaf_count = 0
    };
    for (int i = 0; i < MAX_ROWS; i++) {
        tree.branch_count[i] = 0;
    }
    return tree;
}

#endif // TREE_IMPL
#endif // TREE_H