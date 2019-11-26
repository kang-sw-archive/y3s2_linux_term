/*! \brief All game objects
    \file obj.h
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-26
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
    \details
 */
#pragma once
#include "core/program.h"

enum
{
    FRUIT_BOMB = -1,
};
typedef int EFruitPoint;

typedef struct fruit
{
    FVec2float Position;
    FVec2float Velocity;
    UResource *Display;
    EFruitPoint Point;
} FFruit;

typedef struct widget
{
    /*data*/
    FVec2int Position;

} FWidget;

void UpdateFruit(FFruit *s, UProgramInstance *pinst, float DeltaTime);