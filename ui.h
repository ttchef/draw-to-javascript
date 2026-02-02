
#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <raylib.h>

#include "clay.h"

struct Context;

typedef struct uiState {

} uiState;

void compute_clay_layout(struct Context* ctx, Texture2D* textures, size_t image_count);

#endif // UI_H 
