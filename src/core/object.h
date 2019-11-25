/*! \brief
    \file object.h
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-25
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
    \details
        사용자에게 보일 수 있는 가장 기본 단위의 객체.
 */
#pragma once
#include <stdio.h>
#include "common.h"

typedef struct GameObject UGameObject;

typedef struct GameObject_vtable
{
    void (*Update)(UGameObject *, float /*DeltaTime*/);
    void (*Draw)(UGameObject *, struct ProgramInstance *);
} GameObject_vtable_t;