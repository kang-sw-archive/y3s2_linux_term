#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "utility.h"
typedef int32_t EStatus;

enum
{
    STATUS_OK = 0,
    ERROR_FAILED = -1,
};

// Hash type definition
typedef uint32_t FHash;

// Handle type definition to detach user from implementation
typedef void *FHandle;

#define INVALID_HASH ((uint32_t)-1)
