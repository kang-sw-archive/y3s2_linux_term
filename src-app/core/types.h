#pragma once
#include "common.h"

#define VEC2_TEMPLACE_TYPE(ty) \
    typedef struct vec2_##ty   \
    {                          \
        ty x, y;               \
    } FVec2##ty

VEC2_TEMPLACE_TYPE(int8_t);
VEC2_TEMPLACE_TYPE(int);
VEC2_TEMPLACE_TYPE(int16_t);
VEC2_TEMPLACE_TYPE(int32_t);
VEC2_TEMPLACE_TYPE(int64_t);
VEC2_TEMPLACE_TYPE(float);

typedef struct transform
{
    FVec2float P; // Position
    FVec2float S; // Scale
    float R;      // Rotation
} FTransform2;