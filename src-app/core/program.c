#include <stdio.h>
#include "program.h"
#include "uEmbedded/algorithm.h"

static TYPEID const PInstTypeID = {.TypeName = "ProgramInstance"};
ASSIGN_TYPEID(UProgramInstance, PInstTypeID);

static int resource_eval_func(void const *veval, void const *velem)
{
    int64_t rmres = (int64_t) * ((FHash *)veval) - (int64_t)((struct Resource *)velem)->Hash;
    return rmres > 0 ? 1 : rmres < 0 ? -1 : 0;
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

EStatus PInst_LoadImage(struct ProgramInstance *PInst, FHash Hash, char const *Path)
{
    UResource *rs;
    rs = pinst_resource_find(PInst, Hash);

    if (rs)
        return STATUS_RESOURCE_ALREADY_EXIST;

    void *data = Internal_PInst_LoadImgInternal(PInst, Path);
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
    FRenderEventArg *a = va;
    FRenderEventArg *b = vb;

    return a->Layer - b->Layer;
}

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
    pqueue_init(&inst->RenderEventQueue,
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

    // @todo.
    logprintf("Program has been initialized successfully.\n");

    return inst;
}

static void RenderThread(void *VPInst)
{
    UProgramInstance *inst = VPInst;
}