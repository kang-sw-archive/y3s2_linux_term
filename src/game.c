#include "core/program.h"
#include "uEmbedded/fslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>
#include <cairo.h>

#define countof(v) (sizeof(v) / sizeof(*v))
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// #### DECLARATIONS ####
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
// -- INPUT PROCEDURE
#define EVENT_DEVICE "/dev/input/event0"
#define Max_Input_Event 128
#define MAX_ASYNC_INPUT_EVENT 256

static void *InputProcedure(void *dev);

// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
// -- ALL TYPE DEFINITIONS
enum CurrentGameState
{
    GAME_TITLE = 1,
    GAME_RANKING,
    GAME_OVER,
    GAME_PLAY
};

enum TouchInputType
{
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_MV,
};

typedef struct touchinput
{
    int16_t x, y;
    int8_t type;
    int8_t slot;
} touchinput_t;

// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// --------------------------------------------------------------
int GameState;

// Input data

extern bool g_bRun;
pthread_t ghInputProcThr;
touchinput_t gInputEventQueue[MAX_ASYNC_INPUT_EVENT];
size_t gInputEventSubmitIdx;
size_t gInputEventRecvIdx;
extern UProgramInstance *g_pInst;

// -- WIDGET OBJECT QUEUE

// -- Global Resource Handles
cairo_surface_t *rsrcBackground;
UResource *rsrcDefaultFont;
// --------------------------------------------------------------

// Update methods
void OnDestroyGameInstance();
static void UpdateOnGameTitle(float DeltaTime);
static void UpdateOnGameRanking(float DeltaTime);
static void UpdateOnGameOver(float DeltaTime);
static void UpdateOnGamePlay(float DeltaTime);

// Utility methods
void EnqueueInputEvent(struct touchinput const *ev);
size_t DequeueInputEvent(struct touchinput *ev, size_t max);

// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// #### DEFINITIONS ####
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
// -- GAME UPDATE METHODS
void OnInitGame()
{
    GameState = GAME_TITLE;

    // Initialize Input Device
    // Initialize input event queue
    size_t buffsz = MAX_ASYNC_INPUT_EVENT * (sizeof(touchinput_t) + FSLIST_NODE_SIZE);

    // Before create thread, initialize mutex first.
    pthread_create(&ghInputProcThr, NULL, InputProcedure, EVENT_DEVICE);
    lvlog(LOGLEVEL_INFO, "Successfully initialized input device.\n");

    // Load resources
    PInst_LoadResource(
        g_pInst,
        RESOURCE_FONT,
        hash_djb2("DefaultFont"),
        "Ubuntu Mono",
        LOADRESOURCE_FLAG_FONT_DEFAULT);
    rsrcDefaultFont = PInst_GetResource(g_pInst, hash_djb2("DefaultFont"));
    if (rsrcDefaultFont == NULL)
        lvlog(LOGLEVEL_ERROR, "Failed to load default font resource.\n");
}

void OnDestroyGameInstance()
{
    pthread_join(ghInputProcThr, NULL);
}

void OnUpdate(float DeltaTime)
{
    // Update Game Status
    switch (GameState)
    {
    case GAME_TITLE:
        UpdateOnGameTitle(DeltaTime);
        break;
    case GAME_RANKING:
        UpdateOnGameRanking(DeltaTime);
        break;
    case GAME_OVER:
        UpdateOnGameOver(DeltaTime);
        break;
    case GAME_PLAY:
        UpdateOnGamePlay(DeltaTime);
        break;

    default:
        break;
    }
}

static void UpdateOnGameTitle(float DeltaTime)
{
    static bool bColorSet = false;
    static FColor color_rand[10];
    touchinput_t input[128];
    if (bColorSet == false)
    {
        for (size_t i = 0; i < countof(color_rand); i++)
            color_rand[i] = (FColor){.A = 1, .R = 1, .G = rand() / (float)RAND_MAX, .B = rand() / (float)RAND_MAX};
        bColorSet = true;
    }

    size_t num = DequeueInputEvent(input, countof(input));

    FTransform2 tr = FTransform2_Zero();
    tr.S = (FVec2float){32.0f, 32.0f};

    FColor C = {.A = 1, .R = 1, .G = 0, .B = 0};
    char buff[1024];

    // if (num > 0)
    // printf("---------------- NumEv : %d\n", num);
    for (size_t i = 0; i < num; i++)
    {
        tr.P = PInst_ScreenToWorld(g_pInst, input[i].x, input[i].y);

        // printf(
        //     "\t %d --> (slot %d) [%d %d] -> [%f %f] \n", i,
        //     input[i].slot,
        //     input[i].x, input[i].y,
        //     tr.P.x, tr.P.y);
        sprintf(buff,
                "%d --> (slot %d) [%d %d] -> [%f %f] ", i,
                input[i].slot,
                input[i].x, input[i].y,
                tr.P.x, tr.P.y);
        PInst_RQueueText(g_pInst, 0, &tr, rsrcDefaultFont, buff, color_rand + input[i].slot, true);
        sprintf(buff, "hello, world!");
    }
    sprintf(buff, "hello, world!");
}

static void UpdateOnGameRanking(float DeltaTime)
{
}
static void UpdateOnGameOver(float DeltaTime)
{
}
static void UpdateOnGamePlay(float DeltaTime)
{
}

static void *InputProcedure(void *dev)
{
    int fd = open((char const *)dev, O_RDONLY);
    if (fd == -1)
    {
        lvlog(LOGLEVEL_ERROR, "%s is not a vaild device\n", EVENT_DEVICE);
        g_bRun = false;
    }

    // Input event array
    struct input_event ev[Max_Input_Event];
    struct input_event *ph;
    size_t bytesRead = 0;
    size_t numRead;

    fd_set fdset, rdfdset;
    struct timeval timeout;

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    struct touch_slot
    {
        int16_t x, y;
        bool bPressed;
        bool bPressing;
        bool bUnpressed;
        bool bDirty;
    } slots[10];
    int slot_selection = 0;
    memset(slots, 0, sizeof(slots));
    touchinput_t input;

    while (g_bRun)
    {
        // Check if threr's any data to check
        rdfdset = fdset;
        timeout.tv_sec = 0;
        timeout.tv_usec = 16000;

        switch (select(fd + 1, &rdfdset, NULL, NULL, &timeout))
        {
        case 0:
            continue;
        default:
            break;
        }

        // Read input
        for (bytesRead = 0;;)
        {
            bytesRead += read(fd, (char *)ev + bytesRead, sizeof(*ev));
            numRead = bytesRead / sizeof(*ev);

            if (bytesRead - numRead * sizeof(*ev) != 0)
            {
                lvlog(LOGLEVEL_ERROR, "Invalid size of data %d has read. It must be multiplicand of %d\n", bytesRead, sizeof(*ev));
                g_bRun = false;
                break;
            }

            if (ev[numRead - 1].type == EV_SYN || bytesRead == sizeof(ev))
            {
                --numRead;
                break;
            }
        }

        // static char const *ABS_MSG[] = {"ABS_MT_RESERVED", "ABS_MT_SLOT", "ABS_MT_TOUCH_MAJOR", "ABS_MT_TOUCH_MINOR", "ABS_MT_WIDTH_MAJOR", "ABS_MT_WIDTH_MINOR", "ABS_MT_ORIENTATION", "ABS_MT_POSITION_X", "ABS_MT_POSITION_Y", "ABS_MT_TOOL_TYPE", "ABS_MT_BLOB_ID", "ABS_MT_TRACKING_ID", "ABS_MT_PRESSURE", "ABS_MT_DISTANCE", "ABS_MT_TOOL_X", "ABS_MT_TOOL_Y"};

        ph = ev;
        // logprintf("Processing %d events ... \n", numRead);

        for (; numRead; --numRead, ++ph)
        {
            switch (ph->type)
            {
            case EV_ABS:
                // printf("ABS EVENT RECV: CODE %s\n\t", ABS_MSG[ph->code - 0x2e]);
                switch (ph->code)
                {
                case ABS_MT_PRESSURE:
                    // printf("PRESSURE STATUS TRANSITION %d\n", ph->value);
                    break;
                case ABS_MT_SLOT:
                    // printf("SLOT TRANSITION - %d\n", ph->value);
                    if (ph->value >= 0)
                        slot_selection = ph->value;
                    break;
                case ABS_MT_TRACKING_ID:
                    // printf("TRACKING TRANSITION - slot %d: %d\n", slot_selection, ph->value);
                    if (ph->value != -1)
                    {
                        slots[slot_selection].bPressing = true;
                        slots[slot_selection].bPressed = true;
                    }
                    else
                    {
                        slots[slot_selection].bUnpressed = true;
                    }
                    break;
                case ABS_MT_POSITION_X:
                    // printf("X ABS VAL - %-10d \n", ph->value);
                    slots[slot_selection].x = ph->value;
                    break;
                case ABS_MT_POSITION_Y:
                    // printf("Y ABS VAL - %10d\n", ph->value);
                    slots[slot_selection].y = ph->value;
                    break;
                default:
                    // printf("Unhandled ABS EVENT, for value %d\n", ph->value);
                    break;
                }
                slots[slot_selection].bDirty = true;
                break;
            case EV_SYN:
                // printf("-- SYNC EVENT RECEIVE -- \n");
                // Ignore sync.
                break;
            case EV_KEY:
                // Ignore.
                break;
            default:
                lvlog(LOGLEVEL_INFO, "UNHANDLED EV TYPE: %d\n", ph->type);
                break;
            }
        }

        // Iterate all slots and find slot with dirty flag.
        for (size_t i = 0; i < countof(slots); i++)
        {
            struct touch_slot *s = slots + i;
            if (s->bDirty == false)
                continue;
            // printf("Dirty flag slot %d\n", i);
            input.x = s->x;
            input.y = s->y;
            input.slot = i;

            if (s->bPressing)
            {
                if (s->bUnpressed)
                {
                    // Handle duplicated up event ...
                    input.type = TOUCH_UP;
                    s->bUnpressed = false;
                    s->bPressing = false;
                }
                else
                {
                    input.type = s->bPressed ? TOUCH_DOWN : TOUCH_MV;
                    s->bPressed = false;
                }
            }
            else
            {
                continue;
            }
            s->bDirty = false;

            EnqueueInputEvent(&input);
        }
    }

    int ret = close(fd);
    lvlog(LOGLEVEL_INFO, "Destroied input device. Result: %d\n", ret);
    return NULL;
}

void EnqueueInputEvent(struct touchinput const *ev)
{
    static const size_t idx_add[2] = {1, 1 - countof(gInputEventQueue)};
    // static const char *TOUCHEV_STR[] = {"DOWN", "UP", "MV"};
    // logprintf("slot %d - %s, [%d, %d] \n", ev->slot, TOUCHEV_STR[ev->type], ev->x, ev->y);
    gInputEventQueue[gInputEventSubmitIdx] = *ev;
    gInputEventSubmitIdx += idx_add[gInputEventSubmitIdx == countof(gInputEventQueue) - 1];
}

size_t DequeueInputEvent(struct touchinput *ev, size_t max)
{
    static const size_t idx_add[2] = {1, 1 - countof(gInputEventQueue)};
    size_t out = 0;

    for (;
         out <= max && gInputEventRecvIdx != gInputEventSubmitIdx;
         ++out, gInputEventRecvIdx += idx_add[gInputEventRecvIdx == countof(gInputEventQueue) - 1])
    {
        *ev++ = gInputEventQueue[gInputEventRecvIdx];
    }

    return out;
}