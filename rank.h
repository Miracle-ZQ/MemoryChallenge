#ifndef RANK_H
#define RANK_H

#include "raylib.h"








int saveScore(const char name[], int level, int seconds);







void setCurrentPlayer(const char name[]);







void resetRankPage(void);







int updateRankPage(void);







void drawRankPage(Font font);





#define CUSTOM_RANK_TOP 5







int saveCustomScore(const char name[], int finalScore);







int resetCustomRankPage(void);





int updateCustomRankPage(void);






void drawCustomRankPage(Font font);

#endif
