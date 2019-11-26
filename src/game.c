#include "core/program.h"

// -- ALL STATIC DATA REQUIRED
// --------------------------------------------------------------
enum CurrentGameState
{
    GAME_TITLE = 1,
    GAME_RANKING,
    GAME_OVER,
    GAME_PLAY
} GameState;

// -- WIDGET OBJECT QUEUE

// --------------------------------------------------------------

static void UpdateOnGameTitle(float DeltaTime);
static void UpdateOnGameRanking(float DeltaTime);
static void UpdateOnGameOver(float DeltaTime);
static void UpdateOnGamePlay(float DeltaTime);

void OnInitGame()
{
    GameState = GAME_TITLE;

    // Initialize
}

void OnUpdate(float DeltaTime)
{
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