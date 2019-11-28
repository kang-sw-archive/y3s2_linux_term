#pragma once
#include "../program.h"

/*! \brief Interfaces between hardware and software. */
struct ProgramInstance
{
    LPTYPEID id;

    // Frame buffer handle. Also used to trigger thread shutdown.
    void *hFB;

    // Sound manageer handle.
    void *hSound;

    // Aspect ratio of screen. Used in translation.
    float AspectRatio;

    // Resource management
    struct Resource *arrResource;
    size_t NumResource;
    size_t NumMaxResource;

    // Double buffered draw arg pool
    int ActiveBufferIndex; // 0 or 1.

    // Camera transform for active buff. Immutable for one frame.
    FTransform2 ActiveCameraTransform;

    // Camera tranform for next frame.
    FTransform2 PendingCameraTransform;

    // Renderer status
    int RendererStatus;

    // Rendering event memory pool. Double buffered.
    char *RenderStringPool[RENDERER_NUM_MAX_BUFFER];
    size_t StringPoolHeadIndex[RENDERER_NUM_MAX_BUFFER];
    size_t StringPoolMaxSize;

    // Evenr argument memory pool
    struct RenderEventArg *arrRenderEventArgPool[RENDERER_NUM_MAX_BUFFER];
    size_t PoolHeadIndex[RENDERER_NUM_MAX_BUFFER];
    size_t PoolMaxSize;

    // Priority queue for manage event objects
    pqueue_t arrRenderEventQueue[RENDERER_NUM_MAX_BUFFER];

    // Thread handle of rendering thread
    pthread_t ThreadHandle;

    // Timer functionality
    timer_logic_t Timer;
    size_t TotalTimeMs;
    double TotalTime;

    // Lock rendering
    bool bRenderingLock;
    bool bEnableRendererYield;
};

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
    ERET_NONE = 0, // Nothing
    ERET_TEXT,     // Text
    ERET_POLY,     // Empty Polygon
    ERET_RECT,     // Filled Rectangle
    ERET_IMAGE
} ERenderEventType;

/*! \brief Text rendering event data structure */
struct RenderEventData_Text
{
    // Length of string
    size_t StrLen;
    // Name of this argument will indicate the string address.
    char const *Str;
    UResource *Font;
    FColor rgba;
};

struct RenderEventData_Polylines
{
    struct Resource *PolyLines;
    FColor rgba;
};

struct RenderEventData_Rectangle
{
    int32_t x0, y0;
    int32_t x1, y1;
    FColor rgba;
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