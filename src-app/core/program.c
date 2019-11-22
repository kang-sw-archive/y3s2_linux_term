#include <stdio.h>
#include <stdlib.h>
#include "program.h"
#include "uEmbedded/algorithm.h"

static TYPEID const PInstTypeID = {.TypeName = "ProgramInstance"};
ASSIGN_TYPEID(UProgramInstance, PInstTypeID);

static int resource_eval_func(void const *veval, void const *velem)
{
    int64_t rmres = (int64_t) * ((FHash *)veval) - (int64_t)((struct Resource *)velem)->Hash;
    return rmres > 0 ? 1 : rmres < 0 ? -1 : 0;
}

static FRenderEventArg *pinst_new_renderevent_arg(UProgramInstance *s)
{
    size_t Active = s->ActiveBuffer;
    uassert(s->PoolHeadIndex[Active] < s->PoolMaxSize);
    return s->arrRenderEventArgPool[Active] + (s->PoolHeadIndex[Active]++);
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
    default:
        logprintf("That type of resource is not defined ! \n");
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

static int RenderEventArg_Predicate(void const *va, void const *vb)
{
    FRenderEventArg const *a = va;
    FRenderEventArg const *b = vb;

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

    // Load frame buffer
    Internal_PInst_InitFB(inst, Init->FrameBufferDevFileName);

    // Initialize renderer memory pool
    pqueue_init(&inst->arrRenderEventQueue,
                sizeof(FRenderEventArg *),
                malloc(sizeof(FRenderEventArg *) * Init->NumMaxDrawCall),
                sizeof(FRenderEventArg *) * Init->NumMaxDrawCall,
                RenderEventArg_Predicate);
    inst->PoolMaxSize = Init->NumMaxDrawCall;
    for (size_t i = 0; i < 2; i++)
    {
        inst->arrRenderEventArgPool[i] = malloc(sizeof(FRenderEventArg) * Init->NumMaxDrawCall);
    }

    // Initialize Renderer Thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&inst->ThreadHandle, &attr, RenderThread, inst);
    pthread_attr_destroy(&attr);

    logprintf("Program has been initialized successfully.\n");

    return inst;
}

static void *RenderThread(void *VPInst)
{
    UProgramInstance *inst = VPInst;

    if (inst == NULL)
    {
        logprintf("Invalid argument has delievered.\n");
        return;
    }

    logprintf("Thread verify . . . typename of input argument: %s\n", inst->id->TypeName);
    logprintf("hFB is %p\n", inst->hFB);
    logprintf("Thread has been successfully initialized. \n");
    int ActiveIdx = inst->ActiveBuffer;
    void *hFB = inst->hFB;

    // hFB is escape trigger.
    while (inst->hFB != NULL)
    {
        // Wait until flip request.
        // This is notified via switching active buffer index value.
        if (inst->ActiveBuffer == ActiveIdx)
        {
            pthread_yield(NULL);
            continue;
        }

        ActiveIdx = inst->ActiveBuffer;

        // Consume all queued draw calls
        for (pqueue_t *DrawCallQueue = &inst->arrRenderEventQueue[ActiveIdx]; DrawCallQueue->cnt; pqueue_pop(DrawCallQueue))
        {
            FRenderEventArg const *Arg = pqueue_peek(DrawCallQueue);
            Internal_PInst_Draw(hFB, &Arg);
        }
        Internal_PInst_Flush(hFB);

        // Release memory pools of current active index
        inst->StringPoolHeadIndex[ActiveIdx] = 0;
        inst->PoolHeadIndex[ActiveIdx] = 0;
    }

    logprintf("Thread is shutting down\n");
}

EStatus PInst_Update(struct ProgramInstance *PInst, float DeltaTime)
{
    return STATUS_OK;
}

void PInst_Destroy(struct ProgramInstance *PInst)
{
    PInst->hFB = NULL;

    pthread_join(PInst->ThreadHandle, NULL);
    Internal_PInst_DeinitFB(PInst);

    logprintf("Successfully destroied.\n");
}
