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
    int previewSeconds;
    double k;
} CustomGameConfig;
typedef struct {
    int G;
    int R;
    int W;
    int H;
    double S;
    double t;
    double B_plus;
    double T;
    double T0;
    double Tref;
    double K;
    double F_H;
    double B_minus;
    int finalScore;
} CustomGameResult;
void calculateCustomResult(const CustomGameConfig *cfg, int correct,
    int wrong, int reveals, double elapsed, CustomGameResult *out);
int isEven(const CustomGameConfig *cfg);
void startCustomGame(const CustomGameConfig *cfg);
void getCustomGameResult(CustomGameResult *out);
int isCustomGame(void);
void unloadGameResources(void);
#endif
