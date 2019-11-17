#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "types.h"
#include "uEmbedded/priority_queue.h"

//// APIs ////
struct ProgramInstInitStruct {
    size_t NumMaxResource;
    size_t RenderStringPoolSize;
    size_t NumMaxDrawCall;
    char const *FrameBufferDevFileName;
};
FHandle PInst_Create(struct ProgramInstInitStruct const* Init);
FHandle PInst_Destroy(FHandle PInst);
EStatus PInst_LoadImage(FHandle PInst, FHash Hash, char const *Path);
struct Resource *PInst_GetResource(FHandle PInst, FHash Hash);

EStatus PInst_Update(FHandle PInst, float DeltaTime);

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
    LPTYPEID id;

    // Frame buffer handle.
    void* hFB;
    
    // Resource management
    struct Resource* arrResource;
    size_t NumResource;
    size_t NumMaxResource;
    
    // Double buffered draw arg pool
    bool ActiveBuffer; // 0 or 1.
    
    // Rendering event memory pool. Double buffered.
    char* RenderStringPool[2];
    size_t StringPoolHeadIndex[2];
    size_t StringPoolMaxSize;
    
    // Evenr argument memory pool
    struct RenderEventArg *arrRenderEventArgPool[2];
    size_t PoolHeadIndex[2];
    size_t PoolMaxSize;
    
    // Priority queue for manage event objects
    pqueue_t RenderEventQueue[2];

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
    // Length of string
    size_t StrLen;
    // Name of this argument will indicate the string address.
    char str[4];
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

typedef struct RenderEventArg
{
    int32_t Layer;
    ERenderEventType Type;
    FTransform2 Transform;
    FRenderEventData Data;
} FRenderEventArg;
