#include "core/program.h"
#include "uEmbedded/fslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>

#define TOUCH_EVENT_DEVICE "/dev/input/event0"
#define Max_Input_Event 128
#define MAX_ASYNC_INPUT_EVENT 256
#define countof(v) (sizeof(v) / sizeof(*v))
#define MAX_WIDGETS 32
#define MAX_OBJ 256
extern bool g_bRun;
extern UProgramInstance *g_pInst;

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

typedef struct obj
{
    FVec2float Position;
    FVec2float Velocity;
    UResource *Display;
    // If type is positive value, it means point.
    int Type;
    float CollisionRange;
} FObj;

enum
{
    FRUITTYPE_PARTICLE = 0,
    FRUITTYPE_BOMB = -1,
};

typedef struct widget
{
    /*data*/
    FVec2int Position;
    FVec2int CollisionRange;
    UResource *ImageDefault;
    UResource *ImageClicked;
    const char *Text;
    FColor TextColor;
    // Should not remove any other widgets inside of this function !
    void (*Update)(struct widget *);
    bool (*Trigger)(struct widget *);
    void *Data;
} FWidget;
