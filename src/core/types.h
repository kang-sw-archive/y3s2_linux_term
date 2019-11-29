#pragma once
#include "common.h"
#include <math.h>

/*! \brief  Basic typeid which only defines its typename. 
    \details  Must be located on the top of all managed object.
 */
struct BaseTypeID
{
    const char *TypeName;
};

typedef struct BaseTypeID TYPEID;
typedef struct BaseTypeID const *LPTYPEID;

#define ASSIGN_TYPEID(TYPENAME, VTABLE_INSTANCE_NAME) LPTYPEID TYPEINFOPTR_##TYPENAME = &VTABLE_INSTANCE_NAME
#define CHECK_TYPEID(RESULT_VARIABLE, INSTANCEPTR, TYPENAME)      \
    {                                                             \
        LPTYPEID *INST_PTR___ = (INSTANCEPTR);                    \
        extern LPTYPEID TYPEINFOPTR_##TYPENAME;                   \
        RESULT_VARIABLE = *INST_PTR___ == TYPEINFOPTR_##TYPENAME; \
    }

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

#define VEC2_CAST(to, v) ((FVec2##type){.x = (v).x, .y = (v).y})
#define VEC2_ADD(type, a, b) ((FVec2##type){.x = (a).x + (b).x, .y = (a).y + (b).y})
#define VEC2_SUB(type, a, b) ((FVec2##type){.x = (a).x - (b).x, .y = (a).y - (b).y})
#define VEC2_MUL(type, a, b) ((FVec2##type){.x = (a).x * (b).x, .y = (a).y * (b).y})
#define VEC2_DIV(type, a, b) ((FVec2##type){.x = (a).x / (b).x, .y = (a).y / (b).y})
#define VEC2_DOT(type, a, b) ((a).x * (b).y + (a).y * (b).x)
#define VEC2_NEG(type, a) ((FVec2##type){.x = -(a).x, .y = -(a).y})
#define VEC2_AABB_CHECK(AA, BB, X, Y) ((AA).x <= (X) && (AA).y <= (Y) && (X) < (BB).x && (Y) < (BB).y)
#define VEC2_AMPL(type, a, scalar) ((FVec2##type){.x = (a).x * (scalar), .y = (a).y * (scalar)})

/*! \brief Rotate vector
    \param v Vector to rotate
    \param rad Angle value to rotate
    \return Rotated vector
 */
static inline FVec2float FVec2f_Rotate(FVec2float const *v, float rad)
{
    float s = sinf(rad);
    float c = cosf(rad);
    float x = v->x;
    float y = v->y;

    return (FVec2float){.x = x * c - y * s, .y = x * s + y * c};
}

typedef struct transform
{
    FVec2float P; // Position
    FVec2float S; // Scale
    float R;      // Rotation
} FTransform2;

static inline FTransform2 FTransform2_Zero() { return (FTransform2){.P = {0.0f, 0.0f}, .S = {1.0f, 1.0f}, .R = 0.0f}; }