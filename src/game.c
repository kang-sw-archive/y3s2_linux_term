#include "game.h"
#include <cairo.h>

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
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// -- ALL STATIC DATA REQUIRED
// --------------------------------------------------------------
int gGameState;

// Input data

static pthread_t ghInputProcThr;
static touchinput_t gInputEventQueue[MAX_ASYNC_INPUT_EVENT];
static size_t gInputEventSubmitIdx;
static size_t gInputEventRecvIdx;
static touchinput_t gTouchInput = {.slot = -1};

// -- WIDGET OBJECT QUEUE
static FObj gObjects[MAX_OBJ];
static FWidget gWidgets[MAX_WIDGETS];
static size_t gObjectTop;
static size_t gWidgetTop;

// -- Global Resource Handles
cairo_surface_t *gBackgroundSurface;
UResource *rsrcDefaultFont;
// --------------------------------------------------------------

// Update methods
void OnDestroyGameInstance();
static void InitGameTitle(void);
static void InitGamePlay(void);
static void InitGameOver(void);
static void InitGameRanking(void);
static void UpdateOnGameTitle(float DeltaTime);
static void UpdateOnGameRanking(float DeltaTime);
static void UpdateOnGameOver(float DeltaTime);
static void UpdateOnGamePlay(float DeltaTime);

// Utility methods
static void EnqueueInputEvent(struct touchinput const *ev);
static size_t DequeueInputEvent(struct touchinput *ev, size_t max);

static inline FWidget *NewWidget()
{
    uassert(gWidgetTop < MAX_WIDGETS);
    FWidget *ret = gWidgets + gWidgetTop++;
    ret->Trigger = NULL;
    ret->Text = NULL;
    ret->Update = NULL;
    ret->TextDeltaOnTouch = (FVec2int){0, 0};
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
    // Initialize Input Device
    // Initialize input event queue
    size_t buffsz = MAX_ASYNC_INPUT_EVENT * (sizeof(touchinput_t) + FSLIST_NODE_SIZE);

    // Before create thread, initialize mutex first.
    pthread_create(&ghInputProcThr, NULL, InputProcedure, TOUCH_EVENT_DEVICE);
    lvlog(LOGLEVEL_INFO, "Successfully initialized input device.\n");

    // Load Background Image
    gBackgroundSurface = cairo_image_surface_create_from_png("../resource/image/background.png");

    // Load resources
    PInst_LoadResource(
        g_pInst,
        RESOURCE_FONT,
        hash_djb2("DefaultFont"),
        "Metal",
        LOADRESOURCE_FLAG_FONT_BOLD);
    rsrcDefaultFont = PInst_GetResource(g_pInst, hash_djb2("DefaultFont"));
    if (rsrcDefaultFont == NULL)
        lvlog(LOGLEVEL_ERROR, "Failed to load default font resource.\n");

    lvlog(LOGLEVEL_INFO, "Initializing game session ... \n");

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

        if (w.ImageDefault == NULL)
        {
            lvlog(LOGLEVEL_CRITICAL, "Widget default image must be specified.\n");
            continue;
        }

        // Update Widget.
        if (w.Update)
            w.Update(&w);

        UResource *toDraw = w.ImageDefault;
        bool bOnTouch =
            VEC2_AABB_CHECK(
                VEC2_SUB(int, w.Position, VEC2_AMPL(int, w.CollisionRange, 0.5f)),
                VEC2_ADD(int, w.Position, VEC2_AMPL(int, w.CollisionRange, 0.5f)),
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

        // Convert Transform
        tr.P = PInst_ScreenToWorld(g_pInst, w.Position.x, w.Position.y);

        // Render widget
        tr.S = (FVec2float){0, 0};
        PInst_RQueueImage(
            g_pInst, 10, &tr,
            toDraw, true);

        // If there's text ... render text
        if (w.Text)
        {
            if (bOnTouch)
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

    // -- Update Game Specific Status
    switch (gGameState)
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
    if (bColorSet == false)
    {
        for (size_t i = 0; i < countof(color_rand); i++)
            color_rand[i] = (FColor){.A = 1, .R = 0, .G = rand() / (float)RAND_MAX, .B = rand() / (float)RAND_MAX};
        bColorSet = true;
    }

    touchinput_t in = gTouchInput;
    if (in.slot < 0)
        return;

    FTransform2 tr = FTransform2_Zero();
    tr.S = (FVec2float){32.0f, 32.0f};

    FColor C = {.A = 1, .R = 1, .G = 0, .B = 0};
    char buff[1024];

    // if (num > 0)
    // printf("---------------- NumEv : %d\n", num);

    tr.P = PInst_ScreenToWorld(g_pInst, in.x, in.y);

    // printf(
    //     "\t %d --> (slot %d) [%d %d] -> [%f %f] \n", i,
    //     in.slot,
    //     in.x, in.y,
    //     tr.P.x, tr.P.y);
    sprintf(buff,
            "%d --> (slot %d) [%d %d] -> [%f %f] ", 0,
            in.slot,
            in.x, in.y,
            tr.P.x, tr.P.y);
    PInst_RQueueText(g_pInst, 0, &tr, rsrcDefaultFont, buff, color_rand + in.slot, true, 0);
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
    FHash hash = hash_djb2(Path);
    EStatus v = PInst_LoadResource(
        g_pInst,
        RESOURCE_IMAGE,
        hash,
        Path,
        LOADRESOURCE_IMAGE_DEFAULT);

    if (v == STATUS_OK)
    {
        lvlog(LOGLEVEL_INFO, "Loaded image %s\n", Path);
    }
    else if (v == STATUS_RESOURCE_ALREADY_EXIST)
    {
    }
    else
    {
        lvlog(LOGLEVEL_WARNING, "Failed to load image %s\n", Path);
        return NULL;
    }

    UResource *ret = PInst_GetResource(g_pInst, hash);

    if (ret == NULL)
        lvlog(LOGLEVEL_CRITICAL, "Failed to find resource %s ... hash %d\n", Path, hash);

    return ret;
}

static bool Title_TriggerStart(FWidget *w)
{
    logprintf("Button pressed.\n");
    return true;
}

static void InitGameTitle(void)
{
    ClearAllWidgetObject();

    FWidget *w = NewWidget();
    w->CollisionRange.x = 600;
    w->CollisionRange.y = 120;
    w->Position.x = 400;
    w->Position.y = 600;
    w->ImageDefault = LoadImagePath("../resource/image/btn/botton_rectangle_standard.png");
    w->ImageClicked = LoadImagePath("../resource/image/btn/botton_rectangle_push.png");
    w->Text = "Hello";
    w->TextColor = (FColor){.A = 1, .R = 0.55, .G = 0.23, .B = 0.13};
    w->FontSz = 64.f;
    w->TextDeltaOnTouch = (FVec2int){.x = 0, .y = 20};
    w->Trigger = Title_TriggerStart;

    w = NewWidget();
    w->CollisionRange.x = 600;
    w->CollisionRange.y = 120;
    w->Position.x = 400;
    w->Position.y = 800;
    w->ImageDefault = LoadImagePath("../resource/image/btn/botton_rectangle_standard.png");
    w->ImageClicked = LoadImagePath("../resource/image/btn/botton_rectangle_push.png");
}

static void InitGamePlay(void)
{
}

static void InitGameOver(void)
{
}

static void InitGameRanking(void)
{
}
