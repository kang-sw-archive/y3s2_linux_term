#include "core/program.h"
#include "uEmbedded/fslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>

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

static void InputProcedure(char const *dev);

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
    int x, y;
    int type;
    int idx;
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

void OnDestroyGameInstance();
static void UpdateOnGameTitle(float DeltaTime);
static void UpdateOnGameRanking(float DeltaTime);
static void UpdateOnGameOver(float DeltaTime);
static void UpdateOnGamePlay(float DeltaTime);

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
    // Before create thread, initialize mutex first.
    // pthread_mutex_init(&gEventListLock, NULL);
    // pthread_create(&ghInputProcThr, NULL, InputProcedure, EVENT_DEVICE);
    lvlog(LOGLEVEL_INFO, "Successfully initialized input device.\n");
}

void OnDestroyGameInstance()
{
    // pthread_join(ghInputProcThr, NULL);
    // pthread_mutex_destroy(&gEventListLock);
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

static void InputProcedure(char const *dev)
{
    int fd = open(dev, O_RDONLY);
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
        if (bytesRead % sizeof(*ev) != 0)
        {
            lvlog(LOGLEVEL_ERROR, "Invalid size of data %d has read. It must be multiplicand of %d\n", bytesRead, sizeof(*ev));
            g_bRun = false;
        }
        numRead = bytesRead / sizeof(*ev);

        ph = ev;
        while (numRead--)
        {
            logprintf("Parsing input event ... %d left \n", numRead);
        }
    }

    int ret = close(fd);
    lvlog(LOGLEVEL_INFO, "Destroied input device. Result: %d\n", ret);
}