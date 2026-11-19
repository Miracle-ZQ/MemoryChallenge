#ifndef UI_H
#define UI_H

#include "raylib.h"

int loadUiResources(Font *font);

void unloadUiResources(Font font);

int isButtonClicked(Rectangle bounds);

void drawButton(Rectangle bounds, const char text[], Font font);

Camera2D getUiCamera(void);

int loadBackgrounds(void);

void drawBackground(int index);

void unloadBackgrounds(void);

#endif
