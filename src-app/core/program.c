#include "program.h"

static TYPEID const PInstTypeID = {.TypeName = "ProgramInstance"};

void *PInst_InitFB(char const *fb);

static int RenderEventArg_Predicate(void const *va, void const *vb)
{
    FRenderEventArg *a = va;
    FRenderEventArg *b = vb;

    return a->Layer - b->Layer;
}

FHandle PInst_Create(struct ProgramInstInitStruct const *Init)
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
    inst->hFB = PInst_InitFB;

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
    return inst;
}

static void RenderThread(void* VPInst)
{
    UProgramInstance *inst = VPInst;
    
}