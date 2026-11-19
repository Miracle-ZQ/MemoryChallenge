



#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "raylib.h"

#define NAME_MAX     32
#define PASSWORD_MAX 64



typedef struct
{
    char name[NAME_MAX];
    char password[PASSWORD_MAX];
} User;




void resetAccountPage(int registerMode);







int updateAccountPage(User *user);




void drawAccountPage(Font font);

#endif
