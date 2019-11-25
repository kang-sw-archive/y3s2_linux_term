#include "core/program.h"

enum CurrentGameState
{
    GAME_TITLE = 1,
    GAME_RANKING,
    GAME_OVER,
    GAME_PLAY
} GameState;

static void OnGameTitle(float DeltaTime);
static void OnGameRanking(float DeltaTime);
static void OnGameOver(float DeltaTime);
static void OnGamePlay(float DeltaTime);

void OnRun()
{
}

void OnUpdate(float DeltaTime)
{
    switch (GameState)
    {
    case GAME_TITLE:
        OnGameTitle(DeltaTime);
        break;
    case GAME_RANKING:
        OnGameRanking(DeltaTime);
        break;
    case GAME_OVER:
        OnGameOver(DeltaTime);
        break;
    case GAME_PLAY:
        OnGamePlay(DeltaTime);
        break;

    default:
        break;
    }
}
