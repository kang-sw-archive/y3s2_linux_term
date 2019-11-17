#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "types.h"

//// APIs ////
struct ProgramInstInitStruct {
    size_t NumMaxResource;
    size_t RenderEventMPoolSize;
};
FHandle PInst_Create(struct ProgramInstInitStruct const* Init);
EStatus PInst_LoadImage(FHandle PInst, FHash Hash, char const *Path);
struct Resource *PInst_GetResource(FHandle PInst, FHash Hash);

// Draw APIs
/*! \brief              Notify ProgramInstance that queueing rendering events are done and readied to render output. Output screen will be refreshed as soon as all of the queue is processed.
    \param CamTransform Camera transform to apply.
    \return             STATUS_OK if succeed, else if failed.
 */
EStatus PInst_RequestFlipBuffer(FHandle PInst, FTransform2 const *CamTransform);

EStatus PInst_RQueueText(FHandle PInst, FTransform2 const* Tr, struct Resource *Font, char const *String);
EStatus PInst_RQueuePolygon(FHandle PInst, FTransform2 const *Tr, struct Resource *Vect, uint32_t rgba);
EStatus PInst_RQueueRect(FHandle PInst, FTransform2 const *Tr, FVec2int v0, FVec2int v1, uint32_t rgba);
EStatus PInst_RQueueImage(FHandle PInst, FTransform2 const *Tr, struct Resource *Image);

//! Program status
enum
{
    RENDERER_IDLE = 0,
    RENDERER_BUSY = 1,
    ERROR_RENDERER_INVALID = -1
};

/*! \brief Interfaces between hardware and software. */
typedef struct ProgramInstance
{
    // Frame buffer handle.
    struct _fbg* hFB;
    
    // Resource management
    struct Resource* arrResource;
    size_t NumResource;
    size_t NumMaxResource;
    
    // Rendering event memory pool. Double buffered.
    char* RenderEventMPool[2];
    size_t PoolHeadIndex[2];
    bool ActiveBuffer; // 0 or 1.
    
    
} UProgramInstance;

typedef enum
{
    RESOURCE_LINEVECTOR,
    RESOURCE_IMAGE,
    RESOURCE_FONT
} EResourceType;

struct Resource
{
    /* data */
    uint32_t Hash;
    EResourceType Type;
    void *data;
};

/*! \brief Type of rendering event. */
typedef enum 
{
    ERET_NONE = 0,  // Nothing
    ERET_TEXT,      // Text
    ERET_POLY,      // Empty Polygon
    ERET_RECT,       // Filled Rectangle
    ERET_IMAGE
} ERenderEventType;

/*! \brief Text rendering event data structure */
struct RenderEventData_Text
{
    // Name of this argument will indicate string itself.
    char d[4];
};

struct RenderEventData_Polylines
{
    struct Resource *PolyLines;
    uint8_t rgba[4];
};

struct RenderEventData_Rectangle
{
    int32_t x0, y0;
    int32_t x1, y1;
    uint8_t rgba[4];
};

struct RenderEventData_IMAGE
{
    struct Resource *Image;
};

typedef union {
    struct RenderEventData_Text Text;
    struct RenderEventData_Polylines Poly;
    struct RenderEventData_Rectangle Rect;
    struct RenderEventData_IMAGE Image;
} FRenderEventData;

typedef struct 
{
    int32_t Layer;
    ERenderEventType Type;
    FTransform2 Transform;
    FRenderEventData Data;
} FRenderEvent;
