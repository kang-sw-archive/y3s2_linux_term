/*! \brief
    \file program.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-23
    
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
 */
#include <stdio.h>
#include <stdlib.h>
#include "program.h"
#include "uEmbedded/algorithm.h"
#include "internal/program-types.h"

static TYPEID const PInstTypeID = {.TypeName = "ProgramInstance"};
ASSIGN_TYPEID(UProgramInstance, PInstTypeID);

static int resource_eval_func(void const *veval, void const *velem)
{
    int64_t a = (int64_t) * ((FHash *)veval);
    int64_t b = (int64_t)((struct Resource *)velem)->Hash;
    int64_t rmres = a - b;
    return rmres > 0 ? 1 : rmres < 0 ? -1 : 0;
}

static FRenderEventArg *pinst_new_renderevent_arg(UProgramInstance *s)
{
    size_t Active = s->ActiveBufferIndex;
    uassert(s->PoolHeadIndex[Active] < s->PoolMaxSize);
    return s->arrRenderEventArgPool[Active] + (s->PoolHeadIndex[Active]++);
}

static inline int pinst_next_buff_idx(UProgramInstance const *s)
{
    static const int idxAddVal[2] = {1, -(RENDERER_NUM_MAX_BUFFER - 1)};
    int idx = s->ActiveBufferIndex;
    return idx + idxAddVal[idx == (RENDERER_NUM_MAX_BUFFER - 1)];
}

static struct Resource *pinst_resource_find(UProgramInstance *s, FHash hash);
static struct Resource *pinst_resource_new(UProgramInstance *s, FHash hash)
{
    uassert(s);
    uassert(s->NumResource < s->NumMaxResource);

    // Find place to insert
    size_t idx = lowerbound(s->arrResource, &hash, sizeof(struct Resource), s->NumResource, resource_eval_func);

    // If any resource with same hash already exists ...
    if (idx < s->NumResource && s->arrResource[idx].Hash == hash)
        return NULL;

    struct Resource *resource = array_insert(
        s->arrResource,
        NULL,
        idx,
        sizeof(struct Resource),
        &s->NumResource);

    resource->Hash = hash;
    return resource;
}

float *PInst_AspectRatio(struct ProgramInstance *s)
{
    return &s->AspectRatio;
}

void PInst_SetCameraTransform(struct ProgramInstance *s, FTransform2 const *v)
{
    s->PendingCameraTransform = *v;
}

EStatus PInst_LoadResource(struct ProgramInstance *PInst, EResourceType Type, FHash Hash, char const *Path, LOADRESOURCE_FLAG_T Flag)
{
    UResource *rs;
    rs = pinst_resource_find(PInst, Hash);

    if (rs)
        return STATUS_RESOURCE_ALREADY_EXIST;

    void *data;
    switch (Type)
    {
    case RESOURCE_IMAGE:
        data = Internal_PInst_LoadImgInternal(PInst, Path);
        break;
    case RESOURCE_FONT:
        data = Internal_PInst_LoadFont(PInst, Path, Flag);
        break;
    case RESOURCE_WAV:
        data = Internal_PInst_LoadWav(PInst, Path);
        break;
    default:
        lvlog(LOGLEVEL_WARNING, "That type of resource is not defined ! \n");
        data = NULL;
        break;
    }

    if (data == NULL)
        return ERROR_INVALID_RESOURCE_PATH;

    rs = pinst_resource_new(PInst, Hash);
    rs->Type = RESOURCE_IMAGE;
    rs->data = data;
    return STATUS_OK;
}

static struct Resource *pinst_resource_find(UProgramInstance *s, FHash hash)
{
    size_t idx = lowerbound(s->arrResource, &hash, sizeof(struct Resource), s->NumResource, resource_eval_func);
    if (idx > s->NumResource)
        return NULL;
    struct Resource *ret = s->arrResource + idx;
    return ret->Hash == hash ? ret : NULL;
}

struct Resource *PInst_GetResource(struct ProgramInstance *PInst, FHash Hash)
{
    return pinst_resource_find(PInst, Hash);
}

static int RenderEventArg_Predicate(FRenderEventArg const **va, FRenderEventArg const **vb)
{
    FRenderEventArg const *a = *va;
    FRenderEventArg const *b = *vb;

    return a->Layer - b->Layer;
}

static void *RenderThread(void *VPInst);

struct ProgramInstance *PInst_Create(struct ProgramInstInitStruct const *Init)
{
    // Init as zero.
    UProgramInstance *inst = calloc(1, sizeof(UProgramInstance));
    inst->id = &PInstTypeID;

    inst->arrResource = calloc(Init->NumMaxResource, sizeof(struct Resource));
    inst->NumMaxResource = Init->NumMaxResource;

    inst->StringPoolMaxSize = Init->RenderStringPoolSize;
    for (size_t i = 0; i < 2; i++)
    {
        inst->RenderStringPool[i] = malloc(Init->RenderStringPoolSize);
    }

    // Initialize timer
    size_t timerBuffSz = TIMER_ELEM_SIZE * Init->NumMaxTimer;
    timer_init(&inst->Timer, malloc(timerBuffSz), timerBuffSz);

    // Load frame buffer
    inst->hFB = Internal_PInst_InitFB(inst, Init->FrameBufferDevFileName);

    // Aspect ratio must be set in InitFB function
    uassert(inst->AspectRatio);
    lvlog(LOGLEVEL_INFO, "Aspect ratio value: %f\n", inst->AspectRatio);

    // Initialize sound
    inst->hSound = Internal_PInst_InitSound(inst);

    // Initialize camera transforms
    inst->ActiveCameraTransform = FTransform2_Zero();
    inst->PendingCameraTransform = FTransform2_Zero();

    // Initialize renderer memory pool
    inst->PoolMaxSize = Init->NumMaxDrawCall;
    for (size_t i = 0; i < RENDERER_NUM_MAX_BUFFER; i++)
    {
        pqueue_init(&inst->arrRenderEventQueue[i],
                    sizeof(FRenderEventArg **),
                    malloc(sizeof(FRenderEventArg **) * Init->NumMaxDrawCall),
                    sizeof(FRenderEventArg **) * Init->NumMaxDrawCall,
                    RenderEventArg_Predicate);
        inst->arrRenderEventArgPool[i] = malloc(sizeof(FRenderEventArg) * Init->NumMaxDrawCall);

        lvlog(LOGLEVEL_INFO, "Initializing screen buffer %d\n", i);
        pqueue_t *q = inst->arrRenderEventQueue + i;
        lvlog(LOGLEVEL_INFO,
              "Num Maximum Args: %d ... should be %d\n",
              q->capacity,
              Init->NumMaxDrawCall);
    }

    // Initialize Renderer Thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&inst->ThreadHandle, &attr, RenderThread, inst);
    pthread_attr_destroy(&attr);

    lvlog(LOGLEVEL_INFO, "Program has been initialized successfully.\n");

    return inst;
}

static void *RenderThread(void *VPInst)
{
    UProgramInstance *inst = VPInst;

    if (inst == NULL)
    {
        lvlog(LOGLEVEL_CRITICAL, "Invalid argument has delievered.\n");
        return NULL;
    }

    lvlog(LOGLEVEL_DISPLAY, "Thread verify . . . typename of input argument: %s\n", inst->id->TypeName);
    lvlog(LOGLEVEL_DISPLAY, "hFB is %p\n", inst->hFB);
    lvlog(LOGLEVEL_DISPLAY, "Thread has been successfully initialized. \n");
    int ActiveIdx = inst->ActiveBufferIndex;
    void *hFB = inst->hFB;

    // hFB is escape trigger.
    while (inst->hFB != NULL)
    {
        // Wait until flip request.
        // This is notified via switching active buffer index value.
        if (((volatile UProgramInstance *)inst)->ActiveBufferIndex == ActiveIdx)
        {
            pthread_yield(NULL);
            continue;
        }
        inst->RendererStatus = RENDERER_BUSY;

        // Before draw ...
        Internal_PInst_Predraw(hFB, ActiveIdx);

        // Consume all queued draw calls
        // logprintf("Number of draw calls %d\n", inst->arrRenderEventQueue[ActiveIdx].cnt);
        for (pqueue_t *DrawCallQueue = &inst->arrRenderEventQueue[ActiveIdx]; DrawCallQueue->cnt; pqueue_pop(DrawCallQueue))
        {
            FRenderEventArg const **Arg = pqueue_peek(DrawCallQueue);
            Internal_PInst_Draw(hFB, *Arg, ActiveIdx);
        }
        Internal_PInst_Flush(hFB, ActiveIdx);

        // Release memory pools of current active index
        inst->StringPoolHeadIndex[ActiveIdx] = 0;
        inst->PoolHeadIndex[ActiveIdx] = 0;
        ActiveIdx = inst->ActiveBufferIndex;
        inst->RendererStatus = RENDERER_IDLE;
    }

    lvlog(LOGLEVEL_INFO, "Rendering thread is shutting down\n");
    return NULL;
}

EStatus PInst_UpdateTimer(struct ProgramInstance *PInst, float DeltaTime)
{
    // Update accumulated time.
    PInst->TotalTime += DeltaTime;
    size_t ms = (size_t)(PInst->TotalTime * 1000.0);
    PInst->TotalTimeMs = ms;

    // Update Timer
    timer_update(&PInst->Timer, ms);

    return STATUS_OK;
}

EStatus PInst_Flip(struct ProgramInstance *s)
{
    if (s->RendererStatus != RENDERER_IDLE)
        return RENDERER_BUSY;

    s->ActiveBufferIndex = pinst_next_buff_idx(s);
    s->ActiveCameraTransform = s->PendingCameraTransform;
    lvlog(LOGLEVEL_VERBOSE + 100, "Buffer Successfully Flipped. Active Buffer : %d\n", s->ActiveBufferIndex);

    // Wait until rendering process begin.
    while (((volatile struct ProgramInstance *)s)->RendererStatus == RENDERER_IDLE)
    {
    };
    return STATUS_OK;
}

void PInst_Destroy(struct ProgramInstance *PInst)
{
    void *hFB = PInst->hFB;
    PInst->hFB = NULL;

    pthread_join(PInst->ThreadHandle, NULL);
    Internal_PInst_DeinitFB(PInst, hFB);

    if (PInst->hSound)
        Internal_PInst_DeinitSound(PInst->hSound);

    lvlog(LOGLEVEL_INFO, "Successfully destroied.\n");
}

static bool pinst_push_render_event(UProgramInstance *s, FRenderEventArg *ref)
{
    int active = s->ActiveBufferIndex;
    pqueue_t *queue = &s->arrRenderEventQueue[active];

    if (queue->cnt == queue->capacity)
        return false;

    pqueue_push(queue, &ref);
    return true;
}

#define ANG_TO_RAD (M_PI / 180.0)
#define M_PI 3.14159265358979323846 /* pi */
static void pinst_renderer_translate_camera(FTransform2 *dst, FTransform2 const *obj, FTransform2 const *cam, float Aspect, bool bAbsolute)
{
    // Position should be handled carefully, since its drawing origin changes by camera state.
    if (bAbsolute)
    {
        *dst = *obj;
        dst->P.x += Aspect * 0.5f;
        dst->P.y += 0.5f;
        dst->R *= ANG_TO_RAD;
    }
    else
    {
        FVec2float P = VEC2_SUB(float, obj->P, cam->P);
        P = FVec2f_Rotate(&P, cam->R * ANG_TO_RAD);
        P = VEC2_MUL(float, P, cam->S);

        // To Camera Transform
        P.x += Aspect * 0.5f;
        P.y += 0.5f;

        dst->P = P;
        dst->S = VEC2_MUL(float, cam->S, obj->S);
        dst->R = (cam->R - obj->R) * ANG_TO_RAD;
    }
}

timer_handle_t PInst_QueueTimer(struct ProgramInstance *PInst, void (*Callback)(void *), void *CallbackArg, size_t delay_ms)
{
    return timer_add(&PInst->Timer, PInst->TotalTimeMs + delay_ms, Callback, CallbackArg);
}

bool PInst_AbortTimer(struct ProgramInstance *PInst, timer_handle_t handle)
{
    return timer_erase(&PInst->Timer, handle);
}

size_t PInst_GetTimerDelayLeft(struct ProgramInstance *PInst, timer_handle_t handle)
{
    timer_info_t *info = timer_browse(&PInst->Timer, handle);
    return info ? info->triggerTime - PInst->TotalTimeMs : 0;
}

static FRenderEventArg *pinst_queue_render_event_arg(UProgramInstance *s, int32_t Layer, FTransform2 const *Tr, bool *retv, bool bAbsolute)
{
    FRenderEventArg *ev = pinst_new_renderevent_arg(s);
    pinst_renderer_translate_camera(&ev->Transform, Tr, &s->ActiveCameraTransform, s->AspectRatio, bAbsolute);
    ev->Layer = Layer;
    *retv = pinst_push_render_event(s, ev);
    return ev;
}

EStatus PInst_RQueueImage(struct ProgramInstance *PInst, int32_t Layer, FTransform2 const *Tr, struct Resource *Image, bool bAbsolute)
{
    bool Result;
    FRenderEventArg *ev;
    ev = pinst_queue_render_event_arg(PInst, Layer, Tr, &Result, bAbsolute);

    ev->Data.Image.Image = Image;
    ev->Type = ERET_IMAGE;

    return Result ? STATUS_OK : ERROR_FAILED;
}

EStatus PInst_RQueueText(
    struct ProgramInstance *s,
    int32_t Layer,
    FTransform2 const *Tr,
    struct Resource *Font,
    char const *String,
    COLORREF rgba,
    bool bAbsolute)
{
    bool Result;
    FRenderEventArg *ev;
    ev = pinst_queue_render_event_arg(s, Layer, Tr, &Result, bAbsolute);

    // Copy string.
    int active = s->ActiveBufferIndex;
    char *bf = s->RenderStringPool[active];
    char const *strref = bf;
    size_t head = s->StringPoolHeadIndex[active];
    while (*String)
    {
        bf[head++] = *String++;
    }
    size_t len = &bf[head] - strref;
    bf[head++] = '\0';
    s->StringPoolHeadIndex[active] = head;

    // Setup data
    ev->Data.Text.rgba = *rgba;
    ev->Data.Text.Str = strref;
    ev->Data.Text.StrLen = len;
    ev->Data.Text.Font = Font;

    ev->Type = ERET_TEXT;

    return Result ? STATUS_OK : ERROR_FAILED;
}

EStatus PInst_QueuePlayWave(struct ProgramInstance *PInst, struct Resource *Wav, float Volume)
{
    Internal_PInst_PlayWav(PInst->hSound, Wav->data, Volume);
    return STATUS_OK;
}
