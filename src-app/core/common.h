#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef int32_t EStatus;

enum
{
    STATUS_OK = 0,
    ERROR_FAILED = -1,
};

// Hash type definition
typedef uint32_t FHash;

// Handle type definition to detach user from implementation
typedef struct handle
{
    const char *TypeId;
    void *data;
} FHandle;

#define INVALID_HASH ((uint32_t)-1)