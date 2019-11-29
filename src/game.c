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

// Input data
static pthread_t ghInputProcThr;
static touchinput_t gInputEventQueue[MAX_ASYNC_INPUT_EVENT];
static size_t gInputEventSubmitIdx;
static size_t gInputEventRecvIdx;
static touchinput_t gTouchInput = {.slot = -1};

// -- WIDGET OBJECT QUEUE
static FWidget gWidgets[MAX_WIDGETS];
static size_t gWidgetTop;

// -- Global Resource Handles
cairo_surface_t *gBackgroundSurface;
static UResource *rsrcDefaultFont;
#define FONT_HASH hash_djb2("DefaultFont")
static UResource *rsrcLogo;
static UResource *rsrcDigit[10];
// --------------------------------------------------------------

static void ChangeGameState(void (*UpdateMethod)(float), void (*OnChangeOut)());

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
                VEC2_SUB(int, w.Position, VEC2_AMPL(int, w.Size, 0.5f)),
                VEC2_ADD(int, w.Position, VEC2_AMPL(int, w.Size, 0.5f)),
                gTouchInput.x,
                gTouchInput.y);

        if (bOnTouch && bTouchUp && !bTouchConsumed && w.Trigger)
        {
            bTouchConsumed = w.Trigger(&w);
        }

        if (gTouchInput.slot != -1 && bOnTouch)
        {
            toDraw = w.ImageClicked != NULL ? w.ImageClicked : toDraw;
        }

        if (toDraw)
        {
            // Convert Transform
            tr.P = PInst_ScreenToWorld(g_pInst, w.Position.x, w.Position.y);

            // Render widget
            tr.S = (FVec2float){0, 0};
            PInst_RQueueImage(
                g_pInst, 10, &tr,
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
                g_pInst, 11, &tr, rsrcDefaultFont,
                w.Text, &w.TextColor,
                true, PINST_TEXTFLAG_HALIGN_CENTER | PINST_TEXTFLAG_VALIGN_CENTER);
        }
    }

    if (internal__update_game__)
    {
        internal__update_game__(DeltaTime);
    }
}

static void ChangeGameState(void (*UpdateMethod)(float), void (*OnChangeOut)())
{
    lvlog(LOGLEVEL_INFO, "Changing game state ... \n");
    if (internal__on_change_session__)
        internal__on_change_session__();
    internal__on_change_session__ = OnChangeOut;
    internal__update_game__ = UpdateMethod;
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

#define PATH_IMG_RECT_BUTTON_UP "../resource/image/btn/botton_rectangle_standard.png"
#define PATH_IMG_RECT_BUTTON_DN "../resource/image/btn/botton_rectangle_push.png"
#define PATH_IMG_LOGO "../resource/image/text/TEXT_LOGO.png"

#define PATH_WAV_BGM_TITLE "../resource/wav/bgm/bgm1.wav"

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
    // Load digits before start
    static char const *WAVPATHS[] =
        {
            PATH_WAV_BGM_TITLE,
        };
    static char const *IMGPATHS[] =
        {
            PATH_IMG_RECT_BUTTON_DN,
            PATH_IMG_RECT_BUTTON_UP,
            PATH_IMG_LOGO,
        };

    for (size_t i = 0; i < countof(IMGPATHS); i++)
    {
        LoadImagePath(IMGPATHS[i]);
    }

    for (size_t i = 0; i < countof(WAVPATHS); i++)
    {
        PInst_LoadResource(
            g_pInst,
            RESOURCE_WAV,
            hash_djb2(WAVPATHS[i]),
            WAVPATHS[i],
            0,
            NULL);
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
    ChangeGameState(NULL, NULL);
    ClearAllWidgetObject();

    FWidget *w;

    // Start button
    w = NewWidget();
    w->Size.x = 600;
    w->Size.y = 120;
    w->Position.x = 400;
    w->Position.y = 600;
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
    w->Position.y = 800;
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
    UResource *wav = PInst_GetResource(g_pInst, hash_djb2(PATH_WAV_BGM_TITLE));
    PInst_QueuePlayWave(g_pInst, wav, 1.0f);
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

    ChangeGameState(NULL, NULL);
    ClearAllWidgetObject();

    gPrestartData.disp = NewWidget();
    gPrestartData.disp->ImageDefault = rsrcDigit[gPrestartData.counter];
    gPrestartData.disp->Position = (FVec2int){.x = 400, .y = 400};
    PInst_QueueTimer(g_pInst, PreStartGame_Timer, NULL, 1000);
}

static void PreStartGame_Timer(void *v)
{
    gPrestartData.disp->ImageDefault = rsrcDigit[--gPrestartData.counter];
    lvlog(LOGLEVEL_DISPLAY, "Prestart timer tick. left %d\n", gPrestartData.counter);

    if (gPrestartData.counter == 0)
    {
        // @todo. Init Gameplay Session
        InitGameTitle();
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