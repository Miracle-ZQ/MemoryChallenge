#ifndef GAME_H
#define GAME_H

#include "raylib.h"

void loadGameResources(void);

void startGame(int level);

int updateGame(int *seconds);

int getGameMistakes(void);

int getGameCheats(void);

void drawGame(Font font);

typedef struct {
    int rows;
    int cols;
    double previewSeconds;
} CustomGameConfig;

typedef struct {
    int groups;
    int correct;
    int wrong;
    int reveals;
    int baseScore;
    double elapsedSeconds;
    double boardMultiplier;
    double previewMultiplier;
    double plusMultiplier;
    double minusMultiplier;
    double revealFactor;
    double timePenalty;
    int finalScore;
} CustomGameResult;

int validateCustomConfig(const CustomGameConfig *cfg);

void startCustomGame(const CustomGameConfig *cfg);

void getCustomGameResult(CustomGameResult *out);

int isCustomGame(void);

void unloadGameResources(void);

#endif
