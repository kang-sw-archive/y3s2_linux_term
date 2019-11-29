#include "game.h"
#include <cairo.h>

// #### DECLARATIONS ####
// -- INPUT PROCEDURE
static void *InputProcedure(void *dev);

// -- ALL TYPE DEFINITIONS
// -- ALL STATIC DATA REQUIRED
// --------------------------------------------------------------

// Indicates current game state.
static void (*internal__update_game__)(float delta);
static void (*internal__on_change_session__)();
static void *internal__game_data;

// Input data
static pthread_t ghInputProcThr;
static touchinput_t gInputEventQueue[MAX_ASYNC_INPUT_EVENT];
static size_t gInputEventSubmitIdx;
static size_t gInputEventRecvIdx;
static touchinput_t gTouchInput = {.slot = -1};

// -- WIDGET OBJECT QUEUE
static FWidget gWidgets[MAX_WIDGETS];
static size_t gWidgetTop;
static bool bSessionChangedDuringObjectUpdate;

// -- Global Resource Handles
cairo_surface_t *gBackgroundSurface;
static UResource *rsrcDefaultFont;
#define FONT_HASH hash_djb2("DefaultFont")
static UResource *rsrcLogo;
static UResource *rsrcDigit[10];
// --------------------------------------------------------------

static void *ChangeGameState(void (*UpdateMethod)(float), void (*OnChangeOut)(), size_t);

// Update methods
void OnDestroyGameInstance();
static void InitGameTitle(void);

// Utility methods
static void EnqueueInputEvent(struct touchinput const *ev);
static size_t DequeueInputEvent(struct touchinput *ev, size_t max);

static inline FWidget *NewWidget()
{
    uassert(gWidgetTop < MAX_WIDGETS);
    FWidget *ret = gWidgets + gWidgetTop;
    memset(ret, 0, sizeof(*ret));
    gWidgetTop++;
    return ret;
}

static inline FWidget *DeleteWidget(FWidget *w)
{
    uassert(w - gWidgets < gWidgetTop);
    uassert(w - gWidgets >= 0);
    *w = gWidgets[--gWidgetTop];
}

static inline void ClearAllWidgetObject() { gWidgetTop = 0; }

static UResource *LoadImagePath(char const *Path);

// #### DEFINITIONS ####
// -- GAME UPDATE METHODS
static void LoadAllImage();

void OnInitGame()
{
    // Initialize Input Device
    // Initialize input event queue
    size_t buffsz = MAX_ASYNC_INPUT_EVENT * (sizeof(touchinput_t) + FSLIST_NODE_SIZE);

    // Before create thread, initialize mutex first.
    pthread_create(&ghInputProcThr, NULL, InputProcedure, TOUCH_EVENT_DEVICE);
    lvlog(LOGLEVEL_INFO, "Successfully initialized input device.\n");

    // Load Background Image
    gBackgroundSurface = cairo_image_surface_create_from_png("../resource/image/Background_image.png");

    // Load resources
    PInst_LoadResource(
        g_pInst,
        RESOURCE_FONT,
        FONT_HASH,
        "Metal",
        LOADRESOURCE_FLAG_FONT_BOLD,
        &rsrcDefaultFont);

    if (rsrcDefaultFont == NULL)
        lvlog(LOGLEVEL_ERROR, "Failed to load default font resource.\n");

    lvlog(LOGLEVEL_INFO, "Initializing game session ... \n");

    LoadAllImage();
    InitGameTitle();
}

void OnDestroyGameInstance()
{
    pthread_join(ghInputProcThr, NULL);
}

void OnUpdate(float DeltaTime)
{
    // -- Analyize all inputs then select valid input
    bool bTouchUp = false;
    for (touchinput_t t = {.slot = 0}; DequeueInputEvent(&t, 1);)
    {
        if (t.slot == -1)
            continue;

        if (t.slot == gTouchInput.slot)
            switch (t.type)
            {
            case TOUCH_DOWN:
            case TOUCH_MV:
                gTouchInput = t;
                break;
            case TOUCH_UP:
                gTouchInput.slot = -1;
                bTouchUp = true;
                break;
            }
        else if (gTouchInput.slot == -1)
            switch (t.type)
            {
            case TOUCH_DOWN:
                gTouchInput = t;
                t.slot = -1;
                break;
            default:
                break;
            }
    }

    // -- Update all widgets
    // All widgets will be drawn in layer 10
    bool bTouchConsumed = false;
    bSessionChangedDuringObjectUpdate = false;
    FTransform2 tr = FTransform2_Zero();
    FWidget w;
    for (size_t i = 0; i < gWidgetTop; i++)
    {
        w = gWidgets[i];

        // Update Widget.
        if (w.Update)
            w.Update(&w);

        UResource *toDraw = w.ImageDefault;
        bool bOnTouch =
            VEC2_AABB_CHECK(
                VEC2_SUB(int, w.Position, VEC2_SCALE(int, w.Size, 0.5f)),
                VEC2_ADD(int, w.Position, VEC2_SCALE(int, w.Size, 0.5f)),
                gTouchInput.x,
                gTouchInput.y);

        if (bOnTouch && bTouchUp && !bTouchConsumed && w.Trigger)
        {
            bTouchConsumed = w.Trigger(&w);
            if (bSessionChangedDuringObjectUpdate)
                break;
        }

        if (gTouchInput.slot != -1 && bOnTouch)
        {
            toDraw = w.ImageClicked != NULL ? w.ImageClicked : toDraw;
        }

        // Convert Transform
        tr.P = PInst_ScreenToWorld(g_pInst, w.Position.x, w.Position.y);

        if (toDraw)
        {
            // Render widget
            tr.S = (FVec2float){0, 0};
            PInst_RQueueImage(
                g_pInst, 1000000, &tr,
                toDraw, true);
        }

        if (w.Text)
        {
            // If pressed, calculate text delta location
            if (gTouchInput.slot != -1 && bOnTouch)
            {
                FVec2int np = VEC2_ADD(int, w.Position, w.TextDeltaOnTouch);
                tr.P = PInst_ScreenToWorld(g_pInst, np.x, np.y);
            }

            tr.S = (FVec2float){w.FontSz, w.FontSz};
            PInst_RQueueText(
                g_pInst, 1000001, &tr, rsrcDefaultFont,
                w.Text, &w.TextColor,
                true, PINST_TEXTFLAG_HALIGN_CENTER | PINST_TEXTFLAG_VALIGN_CENTER);
        }
    }

    if (internal__update_game__)
    {
        internal__update_game__(DeltaTime);
    }
}

static inline void *GameData() { return internal__game_data; }

static void *ChangeGameState(void (*UpdateMethod)(float), void (*OnChangeOut)(), size_t ModDataSize)
{
    lvlog(LOGLEVEL_INFO, "Changing game state ... \n");
    bSessionChangedDuringObjectUpdate = true;

    if (internal__on_change_session__)
        internal__on_change_session__();
    if (internal__game_data)
        free(internal__game_data);

    internal__on_change_session__ = OnChangeOut;
    internal__update_game__ = UpdateMethod;

    void *ret = NULL;

    if (ModDataSize)
        ret = malloc(ModDataSize);

    internal__game_data = ret;
    return ret;
}

static void *InputProcedure(void *dev)
{
    int fd = open((char const *)dev, O_RDONLY);
    if (fd == -1)
    {
        lvlog(LOGLEVEL_ERROR, "%s is not a vaild device\n", TOUCH_EVENT_DEVICE);
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

                    // To prevent re-enterring blocked...
                    if (s->bPressed == false)
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
         out < max && gInputEventRecvIdx != gInputEventSubmitIdx;
         ++out, gInputEventRecvIdx += idx_add[gInputEventRecvIdx == countof(gInputEventQueue) - 1])
    {
        *ev++ = gInputEventQueue[gInputEventRecvIdx];
    }

    return out;
}

static UResource *LoadImagePath(char const *Path)
{
    UResource *ret;
    FHash hash = hash_djb2(Path);
    EStatus v = PInst_LoadResource(
        g_pInst,
        RESOURCE_IMAGE,
        hash,
        Path,
        LOADRESOURCE_IMAGE_DEFAULT,
        &ret);

    if (ret)
    {
    }
    else
    {
        lvlog(LOGLEVEL_WARNING, "Failed to load image %s\n", Path);
        return NULL;
    }

    lvlog(LOGLEVEL_DISPLAY, "%s Image [%s] ... For hash %p ... Resource addr %p\n",
          v == STATUS_OK ? "Loaded" : "Find",
          Path, hash, ret);

    return ret;
}

//=====================================================================//
//
// GAME LOADING SESSION
// PATH DECLARATIONS
//
//=====================================================================//

// ---------------------- OTHER    PATHS ------------------------------//
#define PATH_RANKNINGS "../rankings.bin"

// ---------------------- RESOURCE PATHS ------------------------------//
#define PATH_PREFIX_FRUIT "../resource/image/fruit/Fruit_"
#define PATH_PREFIX_EFFECT_SLASH "../resource/image/effect/slash_"
#define PATH_SUFFIX_EFFECT_SLASH "_100.png"

#define PATH_IMG_RECT_BUTTON_UP "../resource/image/btn/botton_rectangle_standard.png"
#define PATH_IMG_RECT_BUTTON_DN "../resource/image/btn/botton_rectangle_push.png"
#define PATH_IMG_LOGO "../resource/image/text/TEXT_LOGO.png"
#define PATH_IMG_PAUSE "../resource/image/btn/button_pause.png"
#define PATH_DEGIT_DOT "../resource/image/num/Num_dot.png"
#define PATH_IMG_FRUIT_BANANA PATH_PREFIX_FRUIT "banana.png"
#define PATH_IMG_FRUIT_APPLE PATH_PREFIX_FRUIT "apple.png"
#define PATH_IMG_FRUIT_BOMB PATH_PREFIX_FRUIT "bomb.png"
#define PATH_IMG_FRUIT_ORANGE PATH_PREFIX_FRUIT "orange.png"
#define PATH_IMG_FRUIT_PINEAPPLE PATH_PREFIX_FRUIT "pineapple.png"
#define PATH_IMG_FRUIT_SHOES PATH_PREFIX_FRUIT "shoes.png"
#define PATH_IMG_FRUIT_SOCKS PATH_PREFIX_FRUIT "socks.png"
#define PATH_IMG_FRUIT_STRAWBERRY PATH_PREFIX_FRUIT "strawberry.png"
#define PATH_IMG_FRUIT_TRASH PATH_PREFIX_FRUIT "trash.png"
#define PATH_IMG_FRUIT_WATERMELON PATH_PREFIX_FRUIT "watermelon.png"

#define PATH_IMG_EFFECT_SLASH_01 PATH_PREFIX_EFFECT_SLASH "Ldiag" PATH_SUFFIX_EFFECT_SLASH
#define PATH_IMG_EFFECT_SLASH_02 PATH_PREFIX_EFFECT_SLASH "lying" PATH_SUFFIX_EFFECT_SLASH
#define PATH_IMG_EFFECT_SLASH_03 PATH_PREFIX_EFFECT_SLASH "Rdiag" PATH_SUFFIX_EFFECT_SLASH
#define PATH_IMG_EFFECT_SLASH_04 PATH_PREFIX_EFFECT_SLASH "stading" PATH_SUFFIX_EFFECT_SLASH

#define PATH_IMG_GAMEOVER "../resource/image/text/TEXT_GAMEOVER.png"
#define PATH_IMG_EFFECT_BOMB "../resource/image/particles/5.png"

#define PATH_IMG_KEY_BTN_UP "../resource/image/btn/round.png"
#define PATH_IMG_KEY_BTN_DN "../resource/image/btn/round_dn.png"

static void RefindDigitImage()
{
    char buff[1024];
    for (size_t i = 0; i < countof(rsrcDigit); i++)
    {
        sprintf(buff, "../resource/image/num/Num_%d.png", i);
        rsrcDigit[i] = LoadImagePath(buff);
        if (rsrcDigit[i] == NULL)
            lvlog(LOGLEVEL_CRITICAL, "WARNING !!! DIGIT IS NOT LOADED CORRECTLY!!");
    }
}

static void LoadAllImage()
{
    static char const *IMGPATHS[] =
        {
            PATH_IMG_RECT_BUTTON_UP,
            PATH_IMG_RECT_BUTTON_DN,
            PATH_IMG_LOGO,
            PATH_IMG_PAUSE,
            PATH_DEGIT_DOT,
            PATH_IMG_FRUIT_BANANA,
            PATH_IMG_FRUIT_APPLE,
            PATH_IMG_FRUIT_BOMB,
            PATH_IMG_FRUIT_ORANGE,
            PATH_IMG_FRUIT_PINEAPPLE,
            PATH_IMG_FRUIT_SHOES,
            PATH_IMG_FRUIT_SOCKS,
            PATH_IMG_FRUIT_STRAWBERRY,
            PATH_IMG_FRUIT_TRASH,
            PATH_IMG_FRUIT_WATERMELON,
            PATH_IMG_EFFECT_SLASH_01,
            PATH_IMG_EFFECT_SLASH_02,
            PATH_IMG_EFFECT_SLASH_03,
            PATH_IMG_EFFECT_SLASH_04,
            PATH_IMG_EFFECT_BOMB,
            PATH_IMG_GAMEOVER,
            PATH_IMG_KEY_BTN_UP,
            PATH_IMG_KEY_BTN_DN,
        };

    for (size_t i = 0; i < countof(IMGPATHS); i++)
    {
        LoadImagePath(IMGPATHS[i]);
    }

    // Generate Digits
    RefindDigitImage(); // Generate

    // Load invalidated references again
    RefindDigitImage(); // Locate
    rsrcDefaultFont = PInst_GetResource(g_pInst, FONT_HASH);
}

//=====================================================================//
//
// GAME TITLE SESSION
//
//=====================================================================//
static void InitPreStartGame(void);
static bool Title_TriggerStart(FWidget *w)
{
    lvlog(LOGLEVEL_INFO, "Game start button pressed.\n");
    InitPreStartGame();
    return true;
}

static void InitGameTitle(void)
{
    lvlog(LOGLEVEL_INFO, "Initializing title screen ...\n");
    ChangeGameState(NULL, NULL, 0);
    ClearAllWidgetObject();

    FWidget *w;

    // Start button
    w = NewWidget();
    w->Position.x = 400;
    w->Position.y = 820;
    w->Size.x = 600;
    w->Size.y = 120;
    w->ImageDefault = LoadImagePath(PATH_IMG_RECT_BUTTON_UP);
    w->ImageClicked = LoadImagePath(PATH_IMG_RECT_BUTTON_DN);
    w->Trigger = Title_TriggerStart;

    w->Text = "Game Start";
    w->TextColor = (FColor){.A = 1, .R = 0.55, .G = 0.23, .B = 0.13};
    w->FontSz = 64.f;
    w->TextDeltaOnTouch = (FVec2int){.x = 0, .y = 20};

    // Rankings Button
    w = NewWidget();
    w->Size.x = 600;
    w->Size.y = 120;
    w->Position.x = 400;
    w->Position.y = 1000;
    w->ImageDefault = LoadImagePath(PATH_IMG_RECT_BUTTON_UP);
    w->ImageClicked = LoadImagePath(PATH_IMG_RECT_BUTTON_DN);

    w->Text = "Rankings";
    w->TextColor = (FColor){.A = 1, .R = 0.55, .G = 0.23, .B = 0.13};
    w->FontSz = 64.f;
    w->TextDeltaOnTouch = (FVec2int){.x = 0, .y = 20};

    // Logo
    w = NewWidget();
    w->Position.x = 400;
    w->Position.y = 200;
    w->ImageDefault = LoadImagePath(PATH_IMG_LOGO);

    // Play sound
}

//=====================================================================//
//
// PRE-STARTING GAME SESSION
//
//=====================================================================//
typedef struct prestart_data
{
    int counter;
    FWidget *disp;
} prestart_data_t;
static prestart_data_t gPrestartData;

static void PreStartGame_Timer(void *);

static void InitPreStartGame(void)
{
    lvlog(LOGLEVEL_INFO, "Initializing pre-start game .. \n");
    gPrestartData.counter = 3;

    ChangeGameState(NULL, NULL, 0);
    ClearAllWidgetObject();

    gPrestartData.disp = NewWidget();
    gPrestartData.disp->ImageDefault = rsrcDigit[gPrestartData.counter];
    gPrestartData.disp->Position = (FVec2int){.x = 400, .y = 400};
    PInst_QueueTimer(g_pInst, PreStartGame_Timer, NULL, 1000);
}

static void InitGameplay(void);
static void PreStartGame_Timer(void *v)
{
    gPrestartData.disp->ImageDefault = rsrcDigit[--gPrestartData.counter];
    lvlog(LOGLEVEL_DISPLAY, "Prestart timer tick. left %d\n", gPrestartData.counter);

    if (gPrestartData.counter == 0)
    {
        InitGameplay();
    }
    else
    {
        PInst_QueueTimer(g_pInst, PreStartGame_Timer, NULL, 1000);
    }
}

//=====================================================================//
//
// GAMEPLAY SESSION
//
//=====================================================================//

// CONSTANTS
#define MIN_SLASH_VELOCITY_SQUARE (1.0f * 1.0f)
#define GRAVITY_CONSTANT_Y 0.7f
#define MAX_INTERVAL 1.0f

enum
{
    INDEX_BOMB = 0,
    INDEX_SOCKS = 1,
    INDEX_SHOES = 2,
    INDEX_TRASH = 3,
    INDEX_FRUITS_BEGIN = 4,
    INDEX_FRUITS_END = 10
};

static const char *gFruitPaths[] = {
    PATH_IMG_FRUIT_BOMB,
    PATH_IMG_FRUIT_SOCKS,
    PATH_IMG_FRUIT_SHOES,
    PATH_IMG_FRUIT_TRASH,
    PATH_IMG_FRUIT_APPLE,
    PATH_IMG_FRUIT_BANANA,
    PATH_IMG_FRUIT_STRAWBERRY,
    PATH_IMG_FRUIT_WATERMELON,
    PATH_IMG_FRUIT_PINEAPPLE,
    PATH_IMG_FRUIT_ORANGE,
};

static UResource *rsrcFruits[countof(gFruitPaths)];
static UResource *rsrcSlashes[4];
static UResource *rsrcParticles[12];
static UResource *rsrcExplosion;

typedef struct obj
{
    // In Game Coordinate
    FVec2float Position;
    FVec2float Velocity;
    UResource *Display;
    // If type is positive value, it means point.
    int Type;
    int Layer;
    float CollisionRange;
    float Lifespan;
} FObj;

enum
{
    FRUITTYPE_PARTICLE = 0,
    FRUITTYPE_BOMB = -1,
};

typedef struct gameplayinfo
{
    FVec2float prevTouch;
    float TimeLeft;
    int Score;
    float Interval;
    float IntervalAcc;
    float LifeSpan;
    char buffScore[256];

    FWidget *wtime[3];
    FWidget *wscore;
    FWidget *wexit;
    FWidget *wpause;

    bool bPaused;

    FObj objects[MAX_OBJ];
    size_t objectTop;
} FGameInfo;

static void UpdateGame(float);
static bool Trigger_Pause(FWidget *w)
{
    InitGameTitle();
    return true;
}

static void InitGameplay(void)
{
    ClearAllWidgetObject();
    FGameInfo *s = ChangeGameState(UpdateGame, NULL, sizeof(FGameInfo));
    memset(s, 0, sizeof(FGameInfo));
    s->TimeLeft = 60.0f;

    FWidget *w;
    // Digits xx.x
    w = NewWidget();
    w->Position = (FVec2int){.x = 190, 110};
    s->wtime[2] = w;

    w = NewWidget();
    w->Position = (FVec2int){.x = 330, 110};
    s->wtime[1] = w;

    w = NewWidget();
    w->Position = (FVec2int){.x = 470, 110};
    w->ImageDefault = LoadImagePath(PATH_DEGIT_DOT);

    w = NewWidget();
    w->Position = (FVec2int){.x = 610, 110};
    s->wtime[0] = w;

    // Pause button
    w = NewWidget();
    w->ImageDefault = LoadImagePath(PATH_IMG_PAUSE);
    w->Position = (FVec2int){.x = 60, .y = 110};
    w->Size = (FVec2int){.x = 120, .y = 120};
    w->Trigger = Trigger_Pause;

    // Score board
    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 1200};
    w->FontSz = 56;
    w->Text = s->buffScore;
    w->TextColor = (FColor){.A = 1, .R = 1, .G = 1, .B = 1};
    s->wscore = w;
    s->Score = 0;

    // Load resources
    for (size_t i = 0; i < countof(rsrcFruits); i++)
        rsrcFruits[i] = LoadImagePath(gFruitPaths[i]);

    rsrcSlashes[0] = LoadImagePath(PATH_IMG_EFFECT_SLASH_01);
    rsrcSlashes[1] = LoadImagePath(PATH_IMG_EFFECT_SLASH_02);
    rsrcSlashes[2] = LoadImagePath(PATH_IMG_EFFECT_SLASH_03);
    rsrcSlashes[3] = LoadImagePath(PATH_IMG_EFFECT_SLASH_04);

    rsrcExplosion = LoadImagePath(PATH_IMG_EFFECT_BOMB);
}

// Game utility
/*! \brief
    \param s 
    \param Lifespan Specify negative value to disable lifespan
    \param Pos 
    \param Vel Initial speed
    \param Type 
    \param Disp 
    \param Collision 
    \return 
 */
static FObj *Game_SpawnObj(FGameInfo *s, int Layer, float Lifespan, FVec2float Pos, FVec2float Vel, int Type, UResource *Disp, float Collision)
{
    if (s->objectTop >= countof(s->objects))
    {
        lvlog(LOGLEVEL_WARNING, "Object overflow\n");
        return NULL;
    }
    FObj *ret = &s->objects[s->objectTop++];
    ret->Position = Pos;
    ret->Layer = Layer;
    ret->Velocity = Vel;
    ret->Type = Type;
    ret->Display = Disp;
    ret->CollisionRange = Collision;
    ret->Lifespan = Lifespan;
    return ret;
}

static float randf(float scale)
{
    return rand() * scale * (1.f / RAND_MAX);
}

static void InitGameOverScreen(int Score);

#define SLASH_EFFECT_VELOCITY_SCALE 0.33f
static void UpdateGame(float delta)
{
    FGameInfo *s = GameData();

    // Update touch ...
    // - Determine collisions
    // - Update scores
    // - Generate effects
    if (gTouchInput.slot != -1)
    {
        // Translate
        FVec2float pos = PInst_ScreenToWorld(g_pInst, gTouchInput.x, gTouchInput.y);

        // Calculate slash speed
        float xd = (pos.x - s->prevTouch.x);
        float yd = (pos.y - s->prevTouch.y);
        float sqval = (xd * xd + yd * yd) / (delta * delta);
        s->prevTouch = pos;

        // Exclude first touch since it is overevaluated beacuse of invalid previous position
        // Evaluate collision only when its speed is higher than threshold.
        if (gTouchInput.type != TOUCH_DOWN && sqval > MIN_SLASH_VELOCITY_SQUARE)
        {
            // Spawn effect on touch location by 4 times.
            for (size_t i = 0; i < 4; i++)
            {
                Game_SpawnObj(s, 15, 0.25f, pos,
                              (FVec2float){(float)rand() / (RAND_MAX / SLASH_EFFECT_VELOCITY_SCALE) - (0.5f * SLASH_EFFECT_VELOCITY_SCALE),
                                           (float)rand() / (RAND_MAX / SLASH_EFFECT_VELOCITY_SCALE) - (0.5f * SLASH_EFFECT_VELOCITY_SCALE)},
                              // (FVec2float){0, 0},
                              FRUITTYPE_PARTICLE, rsrcSlashes[rand() % countof(rsrcSlashes)],
                              0);
            }

            // Check if there's any colliding object
            for (size_t i = 0; i < s->objectTop; i++)
            {
                FObj *obj = s->objects + i;
                if (obj->Type == 0)
                    continue; // Ignore particle touch

                FVec2float objpos = obj->Position;
                xd = pos.x - objpos.x;
                yd = pos.y - objpos.y;
                float distsq = xd * xd + yd * yd;

                if (distsq < (obj->CollisionRange * obj->CollisionRange))
                {
                    // @todo.
                    // On touch any object
                    if (obj->Type == FRUITTYPE_BOMB)
                    {
                        // Spawn bomb effect object
                        Game_SpawnObj(s, 1000, -1, pos, (FVec2float){0, 0}, 0, rsrcExplosion, 0);
                        InitGameOverScreen(s->Score);
                        break;
                    }

                    // Destroy object.
                    *obj = s->objects[--s->objectTop];
                    --i;
                }
            }
        }
    }

    // Randomly spawn object on every interval returns.
    s->IntervalAcc += delta;
    if (s->Interval < s->IntervalAcc)
    {
        s->IntervalAcc = 0;
        // @todo. Add weight to spawn ratio
        s->Interval = MAX_INTERVAL * powf(randf(1.0f), 3.1f);

        int selection = rand() % 10;
        FVec2float loc = (FVec2float){.x = (randf(1.4f) - 0.7f), .y = -0.7f};
        FVec2float vel = VEC2_NEG(float, loc);
        vel.x *= randf(2.5f) - 1.25f;

        Game_SpawnObj(s,
                      10, -1.0f,
                      loc,
                      vel,
                      10, rsrcFruits[selection],
                      0.1);
    }

    // Update objects
    // - Transforms
    // - Lifespan
    float DeltaYVel = delta * GRAVITY_CONSTANT_Y;
    for (size_t i = 0; i < s->objectTop; i++)
    {
        FObj *obj = s->objects + i;

        if (obj->Lifespan > 0)
        {
            obj->Lifespan -= delta;
            if (obj->Lifespan < 0)
            {
                // Destroy object when its lifespan is done.
                *obj = s->objects[--s->objectTop];
                --i;
                continue;
            }
        }

        obj->Velocity.y += DeltaYVel;
        FVec2float DeltaVel = VEC2_SCALE(float, obj->Velocity, delta);
        obj->Position = VEC2_ADD(float, obj->Position, DeltaVel);

        // If object is out of boundary ...
        // - Kill object
        if (obj->Position.y > 0.8f)
        {
            *obj = s->objects[--s->objectTop];
            --i;
            continue;
        }

        // Draw object
        if (obj->Display == NULL)
        {
            lvlog(LOGLEVEL_WARNING, "Object resource is not loaded correctly!\n");
            continue;
        }
        PInst_RQueueImage(g_pInst, obj->Layer, &obj->Position, obj->Display, true);
    }

    // Update time
    s->TimeLeft -= delta;
    if (s->TimeLeft < 0)
    {
        // @todo. Game Over;
        InitGameOverScreen(s->Score);
    }

    // Update fancies
    sprintf(s->buffScore, "SCORE %15d", s->Score);
    int left = (int)(s->TimeLeft * 10.f);
    for (size_t i = 0; i < 3; ++i)
    {
        s->wtime[i]->ImageDefault = rsrcDigit[left % 10];
        left /= 10;
    }
}

//=====================================================================//
//
// GAME OVER SCREEN SESSION
//
//=====================================================================//
struct ranking_value
{
    char name[128];
    int score;
} gRankings[10];

static char gNameEntered[128];
static int gNameCnt;
static int gRecordScore;

static bool Trigger_KeyTouch(FWidget *w)
{
    if (gNameCnt >= countof(gNameEntered) - 1)
        return false;
    gNameEntered[gNameCnt++] = w->Text[0];
    gNameEntered[gNameCnt] = 0;
    return true;
}

static bool Trigger_AcceptScore(FWidget *w)
{

    return true;
}

static void InitGameOverScreen(int Score)
{
    static const char *text[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", ".", "!", "?", ";", ":", "[", "]", "{", "}", "(", ")"};
    static char ScoreBuff[128];
    gRecordScore = Score;
    gNameCnt = 0;
    gNameEntered[0] = '\0';

    // Only disable update
    internal__update_game__ = NULL;
    FWidget *w;

    // Game over text.
    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 240};
    w->ImageDefault = LoadImagePath(PATH_IMG_GAMEOVER);

    // Spawn Keyboard
    size_t KeyboardGap = 80;
    size_t xp = 40;
    size_t yp = 840;
    UResource *rdn = LoadImagePath(PATH_IMG_KEY_BTN_DN);
    UResource *rup = LoadImagePath(PATH_IMG_KEY_BTN_UP);

    for (size_t i = 0; i < countof(text); i++)
    {
        w = NewWidget();
        w->Position = (FVec2int){.x = xp, .y = yp};
        w->Size = (FVec2int){.x = 70, .y = 70};
        w->Text = text[i];
        w->TextColor = (FColor){1, 0.6, 0.6, 0.6};
        w->Trigger = Trigger_KeyTouch;
        w->ImageClicked = rdn;
        w->ImageDefault = rup;
        w->FontSz = 32.0f;
        xp += 80;

        if (i % 10 == 9)
        {
            xp = 40;
            yp += 80;
        }
    }

    // Guide
    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 420};
    w->TextColor = (FColor){.A = 1, .R = 0.88, .G = 0.9, .B = 0.9};
    sprintf(ScoreBuff, "SCORE: %f", Score);
    w->Text = ScoreBuff;
    w->FontSz = 52.0f;

    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 504};
    w->TextColor = (FColor){.A = 1, .R = 0.8, .G = 0.7, .B = 0.8};
    w->Text = "_______________________";
    w->FontSz = 52.0f;

    // Entered text display
    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 500};
    w->TextColor = (FColor){.A = 1, .R = 1, .G = 1, .B = 1};
    w->Text = gNameEntered;
    w->FontSz = 52.0f;

    // Apply button
    w = NewWidget();
    w->Position = (FVec2int){.x = 400, .y = 620};
    w->Size.x = 600;
    w->Size.y = 120;
    w->ImageDefault = LoadImagePath(PATH_IMG_RECT_BUTTON_UP);
    w->ImageClicked = LoadImagePath(PATH_IMG_RECT_BUTTON_DN);
    w->Trigger = Trigger_AcceptScore;

    w->Text = "OK";
    w->TextColor = (FColor){.A = 1, .R = 0.55, .G = 0.23, .B = 0.13};
    w->FontSz = 64.f;
    w->TextDeltaOnTouch = (FVec2int){.x = 0, .y = 20};
}
