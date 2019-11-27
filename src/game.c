#include "core/program.h"
#include "uEmbedded/fslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>

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
#define Max_Input_Event 32
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
    int16_t padding;
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
pthread_mutex_t gEventListLock;
struct fslist gInputEvents;

// -- WIDGET OBJECT QUEUE

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
    fslist_init(&gInputEvents, malloc(buffsz), buffsz, sizeof(touchinput_t));

    // Before create thread, initialize mutex first.
    pthread_mutex_init(&gEventListLock, NULL);
    pthread_create(&ghInputProcThr, NULL, InputProcedure, EVENT_DEVICE);
    lvlog(LOGLEVEL_INFO, "Successfully initialized input device.\n");
}

void OnDestroyGameInstance()
{
    pthread_join(ghInputProcThr, NULL);
    pthread_mutex_destroy(&gEventListLock);

    free(fslist_destroy(&gInputEvents));
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
    static bool stats[10];
    touchinput_t input[32];

    size_t num = DequeueInputEvent(input, countof(input));
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
    int LastEventType = 0;

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
        bytesRead = read(fd, ev, sizeof(ev));
        numRead = bytesRead / sizeof(*ev);

        if (bytesRead - numRead * sizeof(*ev) != 0)
        {
            lvlog(LOGLEVEL_ERROR, "Invalid size of data %d has read. It must be multiplicand of %d\n", bytesRead, sizeof(*ev));
            g_bRun = false;
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
                    // printf("X ABS VAL - %d \n", ph->value);
                    slots[slot_selection].x = ph->value;
                    break;
                case ABS_MT_POSITION_Y:
                    // printf("Y ABS VAL - %d\n", ph->value);
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
            LastEventType = ph->type;
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
    // static const char *TOUCHEV_STR[] = {"DOWN", "UP", "MV"};
    // logprintf("slot %d - %s, [%d, %d] \n", ev->slot, TOUCHEV_STR[ev->type], ev->x, ev->y);

    pthread_mutex_lock(&gEventListLock);
    if (gInputEvents.size >= MAX_ASYNC_INPUT_EVENT)
        fslist_erase(&gInputEvents, &gInputEvents.get[gInputEvents.head]);
    memcpy(fslist_data(&gInputEvents, fslist_insert(&gInputEvents, NULL)), ev, sizeof(*ev));
    pthread_mutex_unlock(&gEventListLock);
}

size_t DequeueInputEvent(struct touchinput *ev, size_t max)
{
    pthread_mutex_lock(&gEventListLock);
    size_t out = gInputEvents.size < max ? gInputEvents.size : max;

    // Copy input events to array
    while (out--)
    {
        struct fslist_node *n = &gInputEvents.get[gInputEvents.head];
        *ev++ = *(touchinput_t *)fslist_data(&gInputEvents, n);
        fslist_erase(&gInputEvents, n);
        static const char *TOUCHEV_STR[] = {"DOWN", "UP", "MV"};
        logprintf("slot %d - %s, [%d, %d] \n", (ev - 1)->slot, TOUCHEV_STR[(ev - 1)->type], (ev - 1)->x, (ev - 1)->y);
    }
    pthread_mutex_unlock(&gEventListLock);

    return out;
}